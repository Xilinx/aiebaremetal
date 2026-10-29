<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Zephyr UART console RX — RESOLVED

Goal: interactive menu over the VEK280 console UART under Zephyr (type `l`/`b`/`q`).
TX worked from the start; **RX never received** — `RXFE` (RX FIFO empty) stayed 1.
This is now fixed in `zephyr/uart_rx_fixup.c`: one-time RX bring-up + prime, a
GICv3 secure-routing fix (`ARE_S`), and interrupt-driven RX feeding a queue read
by `zephyr_console_getline()` in `sw/resnet18_top.cpp`. (A gapless polled reader
also works and is kept documented below as a fallback.)

## Root cause

Two independent problems, both on the board/software side:

1. **SBSA driver does no UART init.** Console = `serial@ff000000` = UART0, 115200,
   compatible `arm,sbsa-uart` (`CONFIG_UART_PL011` + `CONFIG_UART_PL011_SBSA`). In
   SBSA mode every RX/UARTEN/FIFO setup in Zephyr's pl011 driver is gated on
   `!sbsa`, so the driver assumes firmware configured everything. The PLM left TX
   usable but the receiver did not deliver bytes.

2. **The receiver needs priming before it latches bytes.** Even after the
   CR/FIFO bring-up, the external RX line does not deliver bytes until the UART
   is cycled through internal loopback once ("RX prime", `LBE` on → send a byte
   → `LBE` off). After that the byte is held in the RX FIFO normally.

3. **GICv3 doesn't route SPIs to the A72 at EL3.** With interrupt RX enabled,
   an `xsdb` probe showed the UART side is perfect but the interrupt is never
   taken. After one keystroke, with `IMSC = 0x50` (RXIM|RTIM):

   | reg | addr | before key | after key | meaning |
   |---|---|---|---|---|
   | RIS | `0xFF00003C` | `0x25` | `0x65` | bit6 **RTRIS set** — byte arrived, timeout fired |
   | MIS | `0xFF000040` | `0x00` | `0x40` | bit6 **RTMIS asserted** — UART is driving its IRQ |
   | FR  | `0xFF000018` | `0x197` | `0x187` | bit4 **RXFE clears** — byte held in FIFO |

   The UART receives, holds, and raises a masked interrupt — but the ISR never
   runs. Reading the GICD from the A72 core (it's identity-mapped; the DPC and a
   POR'd R5 can't read the secure-banked GICD) gave the smoking gun:

   | reg | addr | value | meaning |
   |---|---|---|---|
   | GICD_CTLR | `0xF9000000` | `0x00000000` | distributor **fully un-enabled**: ARE_S=0, no group enable |
   | GICD_TYPER | `0xF9000004` | `0x007B0425` | 192 INTIDs (INTID 50 valid) |
   | ISENABLER1 | `0xF9000104` | `0x00000000` | (polled build: SPI 50 not enabled) |

   `GICD_CTLR = 0` even though `gicv3_dist_init()` runs, because its secure
   `else` branch sets `EnableGrp1S` **without first setting `ARE_S`** — in GICv3
   `EnableGrp1S` is RES0 while `ARE_S=0`, so the write is dropped and SPI
   `IROUTER` routing is inert. PPIs (timer) work because they're serviced by the
   redistributor + CPU interface, bypassing `GICD_CTLR`.

## The fix (interrupt-driven)

All in `zephyr/uart_rx_fixup.c`, run at boot via
`SYS_INIT(versal_sbsa_uart_rx_init, POST_KERNEL, ...)` (kept in the app tree, out
of the upstream driver, so `west update` can't clobber it):
- **bring-up**: drain `FR.BUSY`, clear `UARTEN`, set `LCRH.FEN`, set `CR.TXE|RXE`
  (+`DTR|RTS`), re-enable `UARTEN`, clear `RSR/ECR`;
- **RX prime**: enter `CR.LBE`, send `0x5a` internally, wait for the echo, exit
  `LBE`, clear `RSR/ECR`, drain. Warns only if the self-test fails;
- **GIC ARE_S fix**: set `GICD_CTLR.ARE_S` then `EnableGrp1S` (GICD is
  identity-mapped at `0xF9000000`), then `irq_disable`/`irq_enable` the UART SPI
  so its `IROUTER` is rewritten with routing now active. Done locally rather than
  patching the Zephyr gicv3 driver;
- **interrupt RX** (`CONFIG_UART_INTERRUPT_DRIVEN=y`): register an
  `uart_irq_callback` + `uart_irq_rx_enable()`; the ISR drains the FIFO with
  `uart_fifo_read()` into a `k_msgq` (`abr_console_rx_q`).

Input read — `zephyr_console_getline()` in `sw/resnet18_top.cpp` blocks on
`abr_console_rx_getc()` (queue read, no busy-spin) and returns a
whitespace/Enter-terminated token like baremetal's `std::cin >> token` (type
`l`/`b`/`q` then Enter). `timeout_ms` bounds only the first wait (headless
auto-run).

### Fallback: gapless polled reader

If the GIC fix is ever undesirable, a polled `zephyr_console_getline()` also
works, but it must be a **tight, gapless poll** of `FR.RXFE`/`DR`: any
`k_msleep()`/`k_busy_wait()`/`uart_poll_in()` gap loses the byte (`RXFE` stays
set). That busy-spins while awaiting a key, which is why interrupt RX is
preferred.

## Confirmed along the way (for future reference)

- Baremetal uses the **same** UART0 and the same PLM/boot image (Zephyr `-bootgen`
  only swaps the A72 partition for `zephyr.elf` at EL3), so MIO/pin-mux is
  identical — not the cause.
- Internal loopback self-test passed (`sent 0x5a got 0x5a`), proving the
  receiver/FIFO/`RXE` and baud are correct; the missing piece was the RX prime.
- The GIC `ARE_S` gap affects **all** SPIs, not just the UART: any SPI-driven
  peripheral would have the same problem at EL3 until `ARE_S` is set.
- No competing input consumer exists (`CONFIG_SHELL`, console handler, the
  console-subsys interrupt handler all off), so nothing else steals bytes.

## Possible follow-ups (optional, not required)

- The `ARE_S` fix is currently local (poking `GICD_CTLR` from `uart_rx_fixup.c`).
  The cleaner home is the Zephyr gicv3 driver's secure branch (set `ARE_S` before
  `EnableGrp1S`) or `CONFIG_GIC_SINGLE_SECURITY_STATE` — worth upstreaming to the
  AMD Zephyr tree if more SPIs are used.
- Fold this bring-up into a small board-specific shim rather than the app, if
  other Zephyr apps need console RX on this target.
