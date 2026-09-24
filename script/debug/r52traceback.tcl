#!/usr/bin/env xsdb
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Call Stack Tracer for ARM R52 in XSDB
# Usage: source traceback.tcl
#        backtrace [elf_file] [stack_depth]

proc parse_register_value {reg_output} {
    # Parse output like "pc: 00146bb0\n" to extract hex value
    # Handle various formats: "pc: 00146bb0" or just "00146bb0"
    set reg_output [string trim $reg_output]
    
    # Remove register name prefix if present (e.g., "pc: ")
    if {[regexp {:\s*([0-9a-fA-F]+)} $reg_output match hex_value]} {
        return [expr "0x$hex_value"]
    } elseif {[regexp {^([0-9a-fA-F]+)$} $reg_output match hex_value]} {
        return [expr "0x$hex_value"]
    } else {
        # Try to extract any hex number
        if {[regexp {([0-9a-fA-F]{8})} $reg_output match hex_value]} {
            return [expr "0x$hex_value"]
        }
    }
    
    # If all else fails, return 0
    return 0
}

proc backtrace {{elf_file ""} {depth 20}} {
    puts "=========================================="
    puts "  ARM R52 Call Stack Trace"
    puts "=========================================="
    puts ""
    
    # Find ELF file if not specified
    if {$elf_file == ""} {
        set elf_files [glob -nocomplain *.elf]
        if {[llength $elf_files] > 0} {
            set elf_file [lindex $elf_files 0]
            puts "Using ELF file: $elf_file"
        }
    }
    
    # Read current registers - parse output to extract hex value
    puts "--- Current Register State ---"
    set pc [parse_register_value [rrd pc]]
    set lr [parse_register_value [rrd lr]]
    set sp [parse_register_value [rrd sp]]
    set cpsr [parse_register_value [rrd cpsr]]
    
    puts [format "PC:   0x%08x" $pc]
    puts [format "LR:   0x%08x" $lr]
    puts [format "SP:   0x%08x" $sp]
    puts [format "CPSR: 0x%08x" $cpsr]
    puts ""
    
    # Check processor mode from CPSR
    set mode [expr {$cpsr & 0x1F}]
    set mode_name "Unknown"
    switch $mode {
        0x10 { set mode_name "User" }
        0x11 { set mode_name "FIQ" }
        0x12 { set mode_name "IRQ" }
        0x13 { set mode_name "Supervisor" }
        0x17 { set mode_name "Abort" }
        0x1B { set mode_name "Undefined" }
        0x1F { set mode_name "System" }
    }
    puts "Processor Mode: $mode_name (0x[format %02x $mode])"
    puts ""
    
    # If in abort mode, get exception details
    if {$mode == 0x17} {
        puts "--- Data Abort Exception Details ---"
        catch {
            set lr_abt [parse_register_value [rrd lr_abt]]
            set spsr_abt [parse_register_value [rrd spsr_abt]]
            set dfsr [parse_register_value [rrd dfsr]]
            set dfar [parse_register_value [rrd dfar]]
            
            set faulting_pc [expr {$lr_abt - 8}]
            
            puts [format "LR_abt:    0x%08x" $lr_abt]
            puts [format "Faulting PC: 0x%08x (lr_abt - 8)" $faulting_pc]
            puts [format "SPSR_abt:  0x%08x" $spsr_abt]
            puts [format "DFSR:      0x%08x" $dfsr]
            puts [format "DFAR:      0x%08x (faulting address)" $dfar]
            
            # Decode DFSR
            set status [expr {$dfsr & 0xF}]
            set fs [expr {($dfsr >> 10) & 0x1}]
            set status [expr {$status | ($fs << 4)}]
            
            puts "\nFault Type: "
            switch $status {
                0x01 { puts "  Alignment fault" }
                0x04 { puts "  Instruction cache maintenance fault" }
                0x0c { puts "  L1 translation, Synchronous external abort" }
                0x0e { puts "  L2 translation, Synchronous external abort" }
                0x05 { puts "  Translation fault, Section" }
                0x07 { puts "  Translation fault, Page" }
                0x09 { puts "  Domain fault, Section" }
                0x0b { puts "  Domain fault, Page" }
                0x0d { puts "  Permission fault, Section" }
                0x0f { puts "  Permission fault, Page" }
                default { puts [format "  Unknown fault (0x%02x)" $status] }
            }
            
            # Look up symbol at faulting address
            if {$elf_file != ""} {
                catch {
                    set sym_info [exec arm-none-eabi-addr2line -e $elf_file -f -p [format "0x%x" $faulting_pc]]
                    puts "\nFaulting Location: $sym_info"
                } err
            }
            puts ""
        } err
    }
    
    puts "--- Call Stack Trace ---"
    puts [format "#0  0x%08x" $pc]
    
    # Resolve symbol for current PC
    if {$elf_file != ""} {
        resolve_symbol $elf_file $pc 0
    }
    
    # Add LR as frame #1
    if {$lr != 0} {
        puts [format "#1  0x%08x (from LR)" $lr]
        if {$elf_file != ""} {
            resolve_symbol $elf_file $lr 1
        }
    }
    
    # Walk the stack
    puts "\n--- Stack Walk ---"
    set frame 2
    set current_sp $sp
    
    # Read stack memory
    catch {
        for {set i 0} {$i < $depth} {incr i} {
            set addr [expr {$current_sp + ($i * 4)}]
            set value [mrd -value -size w $addr]
            
            # Check if this looks like a valid code address
            # ARM code typically in ranges: 0x00000000-0x40000000 for our system
            if {$value >= 0x00000000 && $value <= 0x40000000} {
                # Check if it looks like a return address (should be in .text section)
                if {[is_valid_code_address $elf_file $value]} {
                    puts [format "#%d  0x%08x (from stack at 0x%08x)" $frame $value $addr]
                    if {$elf_file != ""} {
                        resolve_symbol $elf_file $value $frame
                    }
                    incr frame
                }
            }
        }
    } err
    
    puts ""
    puts "=========================================="
    puts "Trace complete. Found $frame frames."
    puts "=========================================="
}

proc resolve_symbol {elf_file addr frame_num} {
    # Try using addr2line first
    catch {
        set result [exec armr5-none-eabi-addr2line -e $elf_file -f -p -a [format "0x%x" $addr]]
        puts "    $result"
    } err1 {
        # Try nm as fallback
        catch {
            set nm_out [exec armr5-none-eabi-nm -n $elf_file]
            set lines [split $nm_out "\n"]
            set best_match ""
            set best_addr 0
            
            foreach line $lines {
                if {[regexp {^([0-9a-f]+)\s+\w\s+(.+)$} $line match sym_addr sym_name]} {
                    set sym_addr_int [expr "0x$sym_addr"]
                    if {$sym_addr_int <= $addr && $sym_addr_int > $best_addr} {
                        set best_addr $sym_addr_int
                        set best_match $sym_name
                    }
                }
            }
            
            if {$best_match != ""} {
                set offset [expr {$addr - $best_addr}]
                puts [format "    in %s + 0x%x" $best_match $offset]
            }
        } err2
    }
}

proc is_valid_code_address {elf_file addr} {
    # Simple heuristic: check if address is aligned and in reasonable range
    if {[expr {$addr & 0x3}] != 0} {
        return 0
    }
    
    # For Thumb mode, addresses have bit 0 set, mask it off
    set addr [expr {$addr & 0xFFFFFFFE}]
    
    # Check if in typical code ranges for our system
    # ATCM: 0x0 - 0x10000
    # DDR:  0x100000 - 0x40000000
    if {($addr >= 0x0 && $addr <= 0x10000) || ($addr >= 0x100000 && $addr <= 0x40000000)} {
        return 1
    }
    
    return 0
}

proc show_registers {} {
    puts "=========================================="
    puts "  ARM R52 Register Dump"
    puts "=========================================="
    
    # General purpose registers
    puts "\n--- General Purpose Registers ---"
    for {set i 0} {$i <= 12} {incr i} {
        set val [parse_register_value [rrd r$i]]
        puts [format "r%-2d: 0x%08x" $i $val]
    }
    
    puts "\n--- Special Registers ---"
    set sp [parse_register_value [rrd sp]]
    set lr [parse_register_value [rrd lr]]
    set pc [parse_register_value [rrd pc]]
    set cpsr [parse_register_value [rrd cpsr]]
    
    puts [format "SP:   0x%08x" $sp]
    puts [format "LR:   0x%08x" $lr]
    puts [format "PC:   0x%08x" $pc]
    puts [format "CPSR: 0x%08x" $cpsr]
    
    # Try to read banked registers
    puts "\n--- Banked Exception Registers ---"
    catch { 
        set lr_abt [parse_register_value [rrd lr_abt]]
        puts [format "LR_abt:   0x%08x" $lr_abt]
    }
    catch {
        set spsr_abt [parse_register_value [rrd spsr_abt]]
        puts [format "SPSR_abt: 0x%08x" $spsr_abt]
    }
    catch {
        set lr_irq [parse_register_value [rrd lr_irq]]
        puts [format "LR_irq:   0x%08x" $lr_irq]
    }
    catch {
        set lr_fiq [parse_register_value [rrd lr_fiq]]
        puts [format "LR_fiq:   0x%08x" $lr_fiq]
    }
    
    puts "\n--- Fault Status Registers ---"
    catch {
        set dfsr [parse_register_value [rrd dfsr]]
        set dfar [parse_register_value [rrd dfar]]
        puts [format "DFSR: 0x%08x (Data Fault Status)" $dfsr]
        puts [format "DFAR: 0x%08x (Data Fault Address)" $dfar]
    }
    catch {
        set ifsr [parse_register_value [rrd ifsr]]
        set ifar [parse_register_value [rrd ifar]]
        puts [format "IFSR: 0x%08x (Instruction Fault Status)" $ifsr]
        puts [format "IFAR: 0x%08x (Instruction Fault Address)" $ifar]
    }
    
    puts ""
}

# Convenience commands
proc bt {{elf ""} {depth 20}} {
    backtrace $elf $depth
}

proc regs {} {
    show_registers
}

puts "============================================"
puts "  ARM R52 Debug Helper Loaded"
puts "============================================"
puts ""
puts "Available commands:"
puts "  backtrace \[elf_file\] \[depth\] - Show call stack trace"
puts "  bt \[elf_file\] \[depth\]        - Alias for backtrace"
puts "  show_registers              - Show all registers"
puts "  regs                        - Alias for show_registers"
puts ""
puts "Example:"
puts "  source traceback.tcl"
puts "  bt vek385.elf 30"
puts "  regs"
puts ""
