# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import re
import os
import sys
## mapping class
class MappingHeaderWriter:
    def __init__(self, header_file="mapping.h"):
        """
        Initialize the writer with the output header file name.
        The mapping is stored in a dictionary.
        """
        self.header_file = header_file
        self.mappings = {}

    def add_mapping(self, name, id_value):
        """
        Add (or update) a mapping from name to id_value.
        :param name: The string name key.
        :param id_value: The integer id value.
        """
        self.mappings[name] = id_value

    def write_header(self):
        """
        Write out the C++ header file containing the mapping.
        The header defines a class 'NameIdMapping' with a method 'getid'
        and a global constant instance 'nameIdMapping' for easy access.
        """
        # Create a header guard based on the header file name.
        header_guard = self.header_file.replace('.', '_').upper()
        lines = []

        # Header guard and includes.
        lines.append(f"#ifndef {header_guard}")
        lines.append(f"#define {header_guard}")
        lines.append("")
        lines.append("#include <string>")
        lines.append("#include <unordered_map>")
        lines.append("")
        
        # Begin class definition.
        lines.append("class NameIdMapping {")
        lines.append("public:")
        # Constructor initializes the unordered_map with our mappings.
        lines.append("    NameIdMapping() : mapping_{")
        # Add each mapping entry.
        for name, id_val in self.mappings.items():
            lines.append(f'        {{"{name}", {id_val}}},')
        lines.append("    } {}")
        lines.append("")
        # Define getid() method.
        lines.append("    int getid(const std::string& name) const {")
        lines.append("        auto it = mapping_.find(name);")
        lines.append("        return (it != mapping_.end()) ? it->second : -1;")
        lines.append("    }")
        lines.append("")
        lines.append("private:")
        lines.append("    std::unordered_map<std::string, int> mapping_;")
        lines.append("};")
        lines.append("")
        # Global instance.
        lines.append("static const NameIdMapping nameIdMapping;")
        lines.append("")
        lines.append(f"#endif // {header_guard}")

        # Write all lines to the header file.
        header_content = "\n".join(lines)
        with open(self.header_file, "w") as f:
            f.write(header_content)
        print(f"Header file '{self.header_file}' written with {len(self.mappings)} mapping(s).")

writer = MappingHeaderWriter("extern_mapping.h")
# Global storage
ddr_names = []
extbuf_ids = []
offsets = []
buffer_descriptors = []
length_values = []

# Helper to insert text after a specific line
def insert_after(lines, index, text):
    lines.insert(index + 1, text + "\n")

# Main patching function
def patch_file(input_file, output_file):
    with open(input_file, 'r') as file:
        lines = file.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i]
        
        # 1. Find adf::getExternalBufferAddress("<ddrname>", ...)
        ddr_match = re.search(r'adf::getExternalBufferAddress\("(.*?)",\s*\d+\)', line)
        if ddr_match:
            ddr_name = ddr_match.group(1)
            ddr_names.append(ddr_name)
            i += 1
            while i < len(lines):
                line = lines[i]
                
                # 2. Find adf::getExternalBufferAddress(<extbufid>)
                extbuf_match = re.search(r'(\w+)\s*\+\s*adf::getExternalBufferAddress\((.*?)\)', line)
                if extbuf_match:
                    num, extbuf_id = extbuf_match.groups()
                    offsets.append(num)
                    extbuf_ids.append(extbuf_id)
                    i += 1
                    while i < len(lines):  
                        line = lines[i]  
                        
                        # Search for something like ".length = XXXX" with variable spaces  
                        length_match = re.search(r'\.length\s*=\s*(.*)', line)    
                        if length_match:  
                            length_val = length_match.group(1)
                            number_match = re.search(r'\d+', length_val)
                            if number_match:
                                length_val = number_match.group(0)
                            length_values.append(length_val)  
                            i += 1  
                            while i < len(lines):
                                line = lines[i]
                                # 3. Find configureBufferDescriptor
                                bd_match = re.search(r'adf::configureBufferDescriptor\s*\(\s*adf::shim_tile\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\)', line)
                                if bd_match:
                                    col, row, bdid, bd = bd_match.groups()
                                    buffer_descriptors.append((col, row, bdid, bd))
                                    
                                    # 4. Insert AddDDRCustomOp after configureBufferDescriptor
                                    if extbuf_ids and ddr_names and length_val:
                                        insert_after(lines, i, f'AddDDRCustomOp(0, {extbuf_ids[-1]}, {offsets[-1]}*4, {col}, {bdid}, "{ddr_names[-1]}", {length_val}, patch_op_code);')
                                        #write the id and name mapping into local file
                                        writer.add_mapping(ddr_names[-1], extbuf_ids[-1])
                                        
                                    break
                                i += 1
                            break
                        i += 1
                    break
                i += 1
        i += 1
    
    # 7-8: Insert patch_op_code as a global variable before executeRuntimeControlInternal
    for i, line in enumerate(lines):
        if 'executeRuntimeControlInternal' in line:
            lines.insert(i, 'int32_t patch_op_code = ( 1<<7) + 1;\n')
            break

    # 9: Add AddDDRCustomOp declaration before executeRuntimeControlInternal
    for i, line in enumerate(lines):
        if 'executeRuntimeControlInternal' in line:
            lines.insert(i, '#include \"aiepatch.h\"\n')
            lines.insert(i + 1, 'void AddDDRCustomOp(uint32_t action, uint64_t bufId, uint64_t offsetInBytes, uint8_t shimCol, uint8_t bdId, std::string bufName,int32_t bufLen, int32_t patch_op_code);\n')
            #lines.insert(i + 2, '#ifndef __TXN_BIN__\n void AddDDRCustomOp(uint32_t action, uint64_t bufId, uint64_t offsetInBytes, uint8_t shimCol, uint8_t bdId, std::string bufName, int32_t patch_op_code) { return; }\n#endif\n')
            break

    # 10: For multi-instance graphs (NUM_INST>=2), aiecompiler partitions the
    # runtime control into per-instance sub-functions plus a shared sub-function
    # that holds the weight-broadcast SOURCE enqueueTasks. The natural sequential
    # call order (Internal0, Internal1, ..., Internal_{N-1}) deadlocks because
    # Internal0's compute blocks waiting for the broadcast that won't be kicked
    # until Internal_{N-1} runs. The fix is a per-layer round-robin interleave
    # so each layer's receiver-configure + source-kick happen as a single
    # grouped sequence (matching how N=1 naturally emits the code).
    # See logs/control_diff/DIFF_SUMMARY.md for the full diff + root cause.
    #
    # Toggle PER_LAYER_INTERLEAVE = False to revert to plain sub-function calls
    # in natural order (useful for A/B debugging).
    PER_LAYER_INTERLEAVE = True

    sub_funcs = []
    for line in lines:
        m = re.match(r'^void (executeRuntimeControlInternal\d+)\(\)\s*$', line)
        if m:
            sub_funcs.append(m.group(1))

    def _find_function_body(lns, fn_name):
        """Return (header_idx, body_start_idx, body_end_idx) for `void fn()`.
        body_start..body_end-1 are the lines strictly INSIDE the {}.
        Returns None if not found."""
        for hi, ln in enumerate(lns):
            if re.match(rf'^void {re.escape(fn_name)}\(\)\s*$', ln):
                bs = hi + 1
                while bs < len(lns) and '{' not in lns[bs]:
                    bs += 1
                if bs >= len(lns):
                    return None
                depth = lns[bs].count('{') - lns[bs].count('}')
                bs += 1
                be = bs
                while be < len(lns) and depth > 0:
                    depth += lns[be].count('{') - lns[be].count('}')
                    if depth == 0:
                        return hi, bs, be
                    be += 1
                return hi, bs, be
        return None

    def _split_by_first_occurrence(lns, body_start, body_end, marker_regex, num_layers):
        """Slice [body_start..body_end) at the FIRST line in each slice that
        matches marker_regex with capture group 1 == layer index str.
        Returns (prologue_lines, [layer_0_lines, ..., layer_{N-1}_lines])."""
        first = {}
        for idx in range(body_start, body_end):
            m = marker_regex.search(lns[idx])
            if m:
                L = int(m.group(1))
                if L not in first:
                    first[L] = idx
        boundaries = []
        for L in range(num_layers):
            if L not in first:
                return None, None  # missing layer — bail
            boundaries.append(first[L])
        # FIRST occurrences MUST be monotonically increasing for the slice to
        # be well-defined; if not, fall through and let caller handle.
        for L in range(num_layers - 1):
            if boundaries[L] >= boundaries[L + 1]:
                return None, None
        boundaries.append(body_end)
        prologue = lns[body_start:boundaries[0]]
        slices = [lns[boundaries[L]:boundaries[L + 1]] for L in range(num_layers)]
        return prologue, slices

    if sub_funcs and PER_LAYER_INTERLEAVE and len(sub_funcs) >= 2:
        # Autodetect: last sub-function is the shared-source one, others are
        # per-instance. (aiecompiler convention for this design.)
        shared_fn = sub_funcs[-1]
        inst_fns = sub_funcs[:-1]
        n_inst = len(inst_fns)

        # Autodetect layer count from any resnet_layers[L] reference.
        max_L = -1
        for ln in lines:
            for m in re.finditer(r'resnet_layers\[(\d+)\]', ln):
                if int(m.group(1)) > max_L:
                    max_L = int(m.group(1))
        num_layers = max_L + 1

        # Per-instance: split by first occurrence of resnet18_<i>.resnet_layers[L]
        inst_prologues = []
        inst_slices_per_inst = []  # list of [slice_0, slice_1, ..., slice_{N-1}]
        for i, fn in enumerate(inst_fns):
            fb = _find_function_body(lines, fn)
            if fb is None:
                inst_prologues = None
                break
            _, bs, be = fb
            marker = re.compile(rf'resnet18_{i}\.resnet_layers\[(\d+)\]')
            pro, sl = _split_by_first_occurrence(lines, bs, be, marker, num_layers)
            if pro is None:
                inst_prologues = None
                break
            inst_prologues.append(pro)
            inst_slices_per_inst.append(sl)

        # Shared: split by first occurrence of compute_graph.wts_ddr[L]
        shared_ok = False
        shared_prologue = None
        shared_slices = None
        if inst_prologues is not None:
            fb = _find_function_body(lines, shared_fn)
            if fb is not None:
                _, bs, be = fb
                marker = re.compile(r'compute_graph\.wts_ddr\[(\d+)\]')
                shared_prologue, shared_slices = _split_by_first_occurrence(
                    lines, bs, be, marker, num_layers
                )
                shared_ok = shared_prologue is not None

        if inst_prologues is not None and shared_ok:
            print(f'[patchddraddr] per-layer interleave: '
                  f'{n_inst} instances x {num_layers} layers + 1 shared source')
            # Build interleaved body. Per-layer order matches what aiecompiler
            # naturally emits for NUM_INST=1: source-kick FIRST, then receivers.
            # See DIFF_SUMMARY.md: in N=1 line 144 enqueues the shim_tile,15
            # source-mm2s BEFORE line 255 configures the wts.in receiver.
            #
            # But N>=2's shared sub-function bundles each layer's source as:
            #   { configure 4 source BDs ; enqueueTask each ; waitDMAChannelDone x4 }
            # The waitDMAChannelDone requires the broadcast to fully drain,
            # which requires BOTH inst-0 AND inst-1 receivers to be armed.
            # So we split shared_slice[L] at the first 'waitDMAChannelDone' (or
            # the 'before wait mm2s channels done' marker emitted just above
            # it) into KICK + WAIT, and emit WAIT only after the inst slices.
            # BDs are reused every layer (only BD IDs 0 and 1 are used on the
            # shim source channels), so layer L's source MUST drain before
            # layer L+1's BD config is written.
            #
            # Further: inst slice's wts.in receiver enqueueTask is followed
            # quickly by a waitDMAChannelDone on the receiver's mem-tile
            # S2MM channel. The broadcast network multicasts to BOTH inst-0
            # AND inst-1 receivers simultaneously, with backpressure if any
            # receiver isn't armed. So inst-0's receiver wait hangs unless
            # inst-1's receiver is already enqueued (= start_bd written) at
            # that point. We further split each inst slice at the end of its
            # last wts.in[*] enqueueTask block into RECV + REST, so both
            # insts' receivers are armed before either's compute waits begin.
            #
            #   shared_prologue  (lock inits for wts_ddr[*])
            #   inst0_prologue   (lock inits / ifm setup for resnet18_0)
            #   inst1_prologue   (... for resnet18_1)
            #   for L in 0..N-1:
            #       shared_slice[L] KICK  (configure + enqueueTask 4 source BDs)
            #       inst0_slice[L] RECV   (writeLayerParams + locks + wts.in[0..3] BDs)
            #       inst1_slice[L] RECV   (same, both receivers now armed -> drain)
            #       inst0_slice[L] REST   (ifm + compute_node + waits)
            #       inst1_slice[L] REST   (same)
            #       shared_slice[L] WAIT  (waitDMAChannelDone x4 on the source)

            def _split_shared_kick_wait(slice_lines):
                """Split a per-layer shared slice into (kick_lines, wait_lines).
                Cut at the first line that mentions 'waitDMAChannelDone' or
                the 'before wait mm2s channels done' ProcessLayerEvent that
                aiecompiler emits just above the wait block."""
                for k, ln in enumerate(slice_lines):
                    if ('waitDMAChannelDone' in ln
                            or 'before wait mm2s channels done' in ln):
                        return slice_lines[:k], slice_lines[k:]
                return slice_lines, []

            def _split_inst_recv_rest(slice_lines, inst_idx, layer_idx):
                """Split an inst-X slice for layer L into (recv_part, rest_part).
                Cut after the closing '}' of the block containing the LAST
                'Enqueue task for ...resnet18_X.resnet_layers[L].wts.in[N]'
                line (N is whatever the max index turns out to be, usually 3).
                If no wts.in marker is found, return (slice, []) (no split)."""
                marker_base = (
                    f'Enqueue task for compute_graph.resnet18_{inst_idx}'
                    f'.resnet_layers[{layer_idx}].wts.in['
                )
                last_marker_line = -1
                for k, ln in enumerate(slice_lines):
                    if marker_base in ln:
                        last_marker_line = k
                if last_marker_line == -1:
                    return slice_lines, []
                depth = 0
                for m in range(last_marker_line, len(slice_lines)):
                    depth += slice_lines[m].count('{') - slice_lines[m].count('}')
                    if depth < 0:
                        return slice_lines[:m + 1], slice_lines[m + 1:]
                return slice_lines, []

            body_parts = []
            body_parts.append(''.join(shared_prologue))
            for pro in inst_prologues:
                body_parts.append(''.join(pro))
            for L in range(num_layers):
                kick, wait = _split_shared_kick_wait(shared_slices[L])
                body_parts.append(''.join(kick))
                recv_parts = []
                rest_parts = []
                for i, sl in enumerate(inst_slices_per_inst):
                    recv, rest = _split_inst_recv_rest(sl[L], i, L)
                    recv_parts.append(recv)
                    rest_parts.append(rest)
                for rp in recv_parts:
                    body_parts.append(''.join(rp))
                for rp in rest_parts:
                    body_parts.append(''.join(rp))
                body_parts.append(''.join(wait))
            interleaved_body = ''.join(body_parts)
            fwd_decls = ''.join(f'void {fn}();\n' for fn in sub_funcs)
            new_body = (
                fwd_decls +
                'void executeRuntimeControlInternal()\n'
                '{\n'
                '#ifdef __AIESIM__\n'
                '    start_parallel_threads();\n'
                '    wait_parallel_threads();\n'
                '#else\n'
                '    // per-layer round-robin interleave of Internal0..N-1 + shared source\n' +
                interleaved_body +
                '#endif\n'
                '\n'
                '}\n'
            )
            # Replace the original wrapper's body
            for i, ln in enumerate(lines):
                if re.match(r'^void executeRuntimeControlInternal\(\)\s*$', ln):
                    brace_count = 0
                    started = False
                    for j in range(i, len(lines)):
                        if '{' in lines[j]:
                            brace_count += lines[j].count('{')
                            started = True
                        if '}' in lines[j]:
                            brace_count -= lines[j].count('}')
                        if started and brace_count == 0:
                            lines[i:j + 1] = [new_body]
                            break
                    break
        else:
            print('[patchddraddr] per-layer interleave FAILED to parse '
                  '(missing layer markers); falling back to natural-order calls')
            PER_LAYER_INTERLEAVE = False  # force fallback below

    if sub_funcs and not PER_LAYER_INTERLEAVE:
        # Fallback: natural-order sequential calls Internal0, Internal1, ..., Internal_{N-1}
        # (Still deadlocks for N>=2 without lock-fence-mode=0; documented for A/B testing.)
        for i, line in enumerate(lines):
            if re.match(r'^void executeRuntimeControlInternal\(\)\s*$', line):
                brace_count = 0
                started = False
                for j in range(i, len(lines)):
                    if '{' in lines[j]:
                        brace_count += lines[j].count('{')
                        started = True
                    if '}' in lines[j]:
                        brace_count -= lines[j].count('}')
                    if started and brace_count == 0:
                        reordered = sub_funcs
                        fwd_decls = ''.join(f'void {fn}();\n' for fn in sub_funcs)
                        calls = ''.join(f'    {fn}();\n' for fn in reordered)
                        new_body = (
                            fwd_decls +
                            'void executeRuntimeControlInternal()\n'
                            '{\n'
                            '#ifdef __AIESIM__\n'
                            '    start_parallel_threads();\n'
                            '    wait_parallel_threads();\n'
                            '#else\n' +
                            calls +
                            '#endif\n'
                            '\n'
                            '}\n'
                        )
                        lines[i:j + 1] = [new_body]
                        break
                break

    # Write patched content to output file
    with open(output_file, 'w') as file:
        file.writelines(lines)

    writer.write_header()

# Usage
aiecontrolcpp= sys.argv[1] if len(sys.argv) > 1 else None
patch_file(aiecontrolcpp, 'output.cpp')
