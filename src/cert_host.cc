// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "errno.h"

#include "aeg_log.h"
#include "barrier.h"
#include "cert_host.h"

#include <iostream>
#include <unistd.h>

extern unsigned char cert_elf[];
extern unsigned int cert_elf_len;

namespace cert_host {

cert_api::cert_api(XAie_DevInst *DevInst) : DevInst(DevInst) { printf("Handshake magic value: 0x%lx\n", ALIVE_MAGIC); }

cert_api::cert_api() {}

cert_api::~cert_api() {
    XAie_MemFree(this->hsa_meminst);
    XAie_MemFree(this->comp_sig_meminst);
}

void cert_api::init_devinst(XAie_DevInst *DevInst) {
    this->DevInst = DevInst;
    AEG_LOG("Handshake value: 0x%lx\n", ALIVE_MAGIC);
}

int cert_api::wakeup_cert(u32 col) {
    AieRC RC;
    XAie_LocType Loc = {.Row = 0, .Col = (u8)col};
    u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;

    AEG_LOG("Waking up cert on col: %d\n", col);
    RC = UcMod->Wakeup(DevInst, Loc, UcMod);
    if (RC) {
        printf("Wakeup failed: %d\n", RC);
    }

    return static_cast<int>(RC);
}

int cert_api::wakeup_cert() {
    int start_col = DevInst->StartCol;
    int end_col = DevInst->StartCol + DevInst->NumCols;
    u32 i;
    int ret = 0;

    for (i = start_col; i < end_col; i++)
        ret |= wakeup_cert(i);

    return ret;
}

int cert_api::load_cert() {
    int ret;

    for (int col = DevInst->StartCol; col < DevInst->StartCol + DevInst->NumCols; col++) {
        ret = load_cert(col);
        if (ret)
            return ret;
    }

    return 0;
}

int cert_api::load_cert(u32 col) {
    printf("Loading cert in col: %d cert_elf: 0x%llx cert_elf_len: %d\n", col,
           reinterpret_cast<unsigned long long>(cert_elf), cert_elf_len);

    AieRC RC;
    XAie_LocType Loc = {.Row = 0, .Col = (u8)col};
    u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;
    u32 CoreStatus = 0;

    if (cert_elf_len == 0) {
        printf("Cert elf len zero.Cannot load cert.\n");
        return -1;
    }
    RC = UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
    if (RC)
        printf("GetCoreStaute failed: %d\n", RC);
    printf("[%d]: CoreStatus: %d\n", Loc.Col, CoreStatus);

    printf("Loading cert in col: %d\n", Loc.Col);
    RC = XAie_LoadUcMem(DevInst, Loc, cert_elf);
    if (RC)
        printf("Load Cert in col %d failed: %d\n", Loc.Col, RC);
    RC = UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
    if (RC)
        printf("GetCoreStaute failed: %d\n", RC);
    printf("[%d]: CoreStatus: %d\n", Loc.Col, CoreStatus);

    return 0;
}

void cert_api::run_cert(u32 cycles) {
    XAie_LocType Loc = {};
    u32 end_col = DevInst->StartCol + DevInst->NumCols;

    while (cycles--) {
        for (Loc.Col = DevInst->StartCol; Loc.Col < end_col; Loc.Col++) {
            u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
            const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;
            u32 CoreStatus = 0;

            (void)UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
        }
    }
}

int cert_api::init_handshake() {
    XAie_LocType Loc = {.Row = 0, .Col = 0};
    AieRC RC;
    struct hsa_queue *hsa_q;
    struct hsa_queue *hsa_q_dma;
    uint64_t hsa_q_dma_addr;
    uint32_t *comp_sig_va;

    this->hsa_meminst = XAie_MemAllocate(this->DevInst, sizeof(struct hsa_queue), XAIE_MEM_NONCACHEABLE);
    if (!this->hsa_meminst) {
        printf("Failed to allocate hsa queue.\n");
        return -1;
    }

    this->comp_sig_meminst = XAie_MemAllocate(this->DevInst, sizeof(uint32_t) * HSA_QUEUE_ENTRY, XAIE_MEM_NONCACHEABLE);
    if (!this->comp_sig_meminst) {
        printf("completion signal alloc failed.\n");
        goto free_hsa_meminst;
    }

    comp_sig_va = (uint32_t *)XAie_MemGetVAddr(this->comp_sig_meminst);
    memset(comp_sig_va, 0, sizeof(*comp_sig_va) * HSA_QUEUE_ENTRY);

    hsa_q = (struct hsa_queue *)XAie_MemGetVAddr(this->hsa_meminst);
    memset(hsa_q, 0, sizeof(*hsa_q));
    hsa_q_dma_addr = XAie_MemGetDevAddr(this->hsa_meminst);
    hsa_q_dma = (struct hsa_queue *)hsa_q_dma_addr;
    hsa_q->hq_header.capacity = HSA_QUEUE_ENTRY;
    hsa_q->hq_header.data_address = (uint64_t)&hsa_q_dma->hq_entry[0];
    XAie_MemSyncForDev(this->hsa_meminst);

    memset(&this->handshake, 0, sizeof(this->handshake));
    handshake.mpaie_alive = ALIVE_MAGIC;
    handshake.partition_base_address = DevInst->StartCol << DevInst->DevProp.ColShift;
    handshake.aie_info.partition_size = DevInst->NumCols;
    for (Loc.Col = 0; Loc.Col < this->DevInst->NumCols; Loc.Col++) {
        if (Loc.Col == 0) {
            handshake.hsa_addr_high = (hsa_q_dma_addr >> 32) & 0xFFFFFFFFUL;
            handshake.hsa_addr_low = hsa_q_dma_addr & 0xFFFFFFFFUL;
        } else {
            handshake.hsa_addr_high = 0xFFFFFFFFUL;
            handshake.hsa_addr_low = 0xFFFFFFFFUL;
        }

        RC = XAie_DataMemBlockWrite(DevInst, Loc, 0 /* Addr */, &handshake, sizeof(handshake));
        if (RC != XAIE_OK) {
            printf("Init of handshake failed: %d", static_cast<int>(RC));
        } else {
        }
    }
    this->parse_host_args();

    return 0;

free_hsa_meminst:
    XAie_MemFree(this->hsa_meminst);

    return -ENOMEM;
}

void cert_api::parse_host_args() {
    uint32_t *host_args;
    uint64_t host_args_dev_addr;
    int i = 0;
    AieRC RC;

    if (!this->external_buffer_map.size())
        goto out;

    this->host_args_meminst =
        XAie_MemAllocate(this->DevInst, this->external_buffer_map.size() * sizeof(uint64_t), XAIE_MEM_NONCACHEABLE);
    if (!this->host_args_meminst) {
        printf("Failed to allocate mem for host args.\n");
        goto out;
    }

    host_args = static_cast<uint32_t *>(XAie_MemGetVAddr(this->host_args_meminst));
    for (auto &[xrt_id, sim_id] : this->external_buffer_map) {
        uint64_t addr = this->external_buffer_addr_map[sim_id];

        /* Addr High */
        host_args[i++] = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
        /* Addr Low */
        host_args[i++] = (uint32_t)(addr & 0xFFFFFFFF);
    }

    host_args_dev_addr = XAie_MemGetDevAddr(this->host_args_meminst);
    this->host_args_high = (uint32_t)((host_args_dev_addr >> 32) & 0xFFFFFFFFUL);
    this->host_args_low = (uint32_t)(host_args_dev_addr & 0xFFFFFFFFUL);
    RC = XAie_MemSyncForDev(this->host_args_meminst);
    if (RC != XAIE_OK) {
        printf("host args sync for dev failed: %d\n", RC);
        goto out;
    }

    return;
out:
    this->host_args_low = 0;
    this->host_args_high = 0;
    this->host_args_meminst = NULL;
}

int cert_api::submit_command(char *ctrl_elf, unsigned int ctrl_elf_len, std::map<std::string_view, uint64_t> &symbol) {
    class cert_elf_loader cel(ctrl_elf, ctrl_elf_len, symbol);
    struct hsa_queue *hsa_q = (struct hsa_queue *)XAie_MemGetVAddr(this->hsa_meminst);
    struct hsa_queue *hsa_q_dma_addr = (struct hsa_queue *)XAie_MemGetDevAddr(this->hsa_meminst);
    struct host_queue_header *hq_header = &hsa_q->hq_header;
    struct host_queue_packet *hq_entry;
    uint32_t *comp_sig_va;
    uint64_t comp_sig_dma;
    uint64_t write_index;

    XAie_MemSyncForCPU(this->hsa_meminst);
    write_index = hq_header->write_index;

    hq_entry = &hsa_q->hq_entry[write_index];

    comp_sig_va = (uint32_t *)XAie_MemGetVAddr(this->comp_sig_meminst);
    comp_sig_va = &comp_sig_va[write_index];
    comp_sig_dma = XAie_MemGetDevAddr(this->comp_sig_meminst);
    comp_sig_dma += sizeof(uint32_t) * write_index;
    XAie_MemSyncForCPU(this->comp_sig_meminst);
    *comp_sig_va = 0;
    XAie_MemSyncForDev(this->comp_sig_meminst);

    this->wakeup_cert();

    cel.parse();
    printf("ccode.cols.size: %ld\n", cel.ccode.cols.size());
    if (cel.ccode.cols.size() > 0) {
        printf("ccode.cols.code.size: %ld\n", cel.ccode.cols[0].code.size());
    }
    if ((cel.ccode.cols.size() > 24) || (cel.ccode.cols.size() == 0)) {
        printf("Cols greater than 24 or 0 not supported\n");
        return -1;
    } else if (cel.ccode.cols.size() == 1) {
        /* Case 1 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        XAie_MemInst *ctrl_code_meminst;
        void *ctrl_code;
        uint64_t ctrl_code_dma_addr;
        struct exec_buf *exec_buf;

        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, cel.ccode.cols[0].code.size(), XAIE_MEM_NONCACHEABLE);
        ctrl_code = (void *)XAie_MemGetVAddr(ctrl_code_meminst);
        ctrl_code_dma_addr = XAie_MemGetDevAddr(ctrl_code_meminst);
        if (!ctrl_code) {
            printf("mem alloc of ctrl code failed\n");
            return -1;
        }
        memcpy(ctrl_code, cel.ccode.cols[0].code.data(), cel.ccode.cols[0].code.size());
        XAie_MemSyncForDev(ctrl_code_meminst);
        printf("ctrl_code: 0x%llx size: %d ctrl_code_dma_addr: 0x%llx\n", ctrl_code, cel.ccode.cols[0].code.size(),
               ctrl_code_dma_addr);
        hexdump(ctrl_code, cel.ccode.cols[0].code.size());

        memset(hq_entry, 0, sizeof(*hq_entry));
        xrt_header->common_header.type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
        xrt_header->common_header.opcode = HOST_QUEUE_PACKET_EXEC_BUF;
        xrt_header->common_header.count = sizeof(struct exec_buf);
        printf("cound = %d\n", xrt_header->common_header.count);
        xrt_header->common_header.distribute = 0;
        xrt_header->common_header.indirect = 0;
        xrt_header->completion_signal = comp_sig_dma;
        exec_buf = (struct exec_buf *)((char *)hq_entry + sizeof(struct xrt_packet_header));
        exec_buf->cu_index = 0;
        exec_buf->dpu_control_code_host_addr_high = (ctrl_code_dma_addr >> 32) & 0xFFFFFFFFUL;
        exec_buf->dpu_control_code_host_addr_low = ctrl_code_dma_addr & 0xFFFFFFFFUL;

        printf("hq_entry: 0x%llx\n", (unsigned long long)(&hsa_q_dma_addr->hq_entry[hq_header->write_index]));
        hexdump(hq_entry, sizeof(*hq_entry));
        if (this->external_buffer_map.size()) {
            exec_buf->args_host_addr_high = this->host_args_high;
            exec_buf->args_host_addr_low = this->host_args_low;
            exec_buf->args_len = this->external_buffer_map.size() * sizeof(uint64_t);
        }
        XAie_MemSyncForDev(this->hsa_meminst);
        hexdump(hq_header, sizeof(*hq_header));
        write_index = (write_index + 1) & (HSA_QUEUE_ENTRY - 1);
        smp_wmb();
        hq_header->write_index = write_index;
        XAie_MemSyncForDev(this->hsa_meminst);
        hexdump(hq_header, sizeof(*hq_header));
        do {
            XAie_MemSyncForCPU(this->comp_sig_meminst);
            printf("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffff);
        } while (!READ_ONCE(comp_sig_va));
        XAie_MemFree(ctrl_code_meminst);
    } else if (cel.ccode.cols.size() <= HSA_MAX_LEVEL1_INDIRECT_ENTRIES) {
        /* case 3 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        struct host_indirect_packet_entry *indirect;
        XAie_MemInst *entry_meminst;
        char *entry_va;
        uint64_t entry_dma;
        XAie_MemInst *ctrl_code_meminst;
        size_t ctrl_size = 0;

        indirect = (struct host_indirect_packet_entry *)&hq_entry->data[0];

        entry_meminst = XAie_MemAllocate(
            this->DevInst, (cel.ccode.cols.size() * (sizeof(struct common_header) + sizeof(struct exec_buf))),
            XAIE_MEM_NONCACHEABLE);
        entry_va = (char *)XAie_MemGetVAddr(entry_meminst);
        entry_dma = XAie_MemGetDevAddr(entry_meminst);
        if (!entry_meminst) {
            printf("entries alloc failed.\n");

            return -1;
        }

        for (auto &[col, col_code] : cel.ccode.cols) {
            ctrl_size += col_code.code.size();
        }
        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, ctrl_size, XAIE_MEM_NONCACHEABLE);
        if (!ctrl_code_meminst) {
            printf("ctrl code meminst alloc failed.\n");
            XAie_MemFree(entry_meminst);

            return -1;
        }

        memset(hq_entry, 0, sizeof(*hq_entry));
        xrt_header->common_header.type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
        xrt_header->common_header.opcode = HOST_QUEUE_PACKET_EXEC_BUF;
        xrt_header->common_header.count = cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry);
        xrt_header->common_header.distribute = 1;
        xrt_header->common_header.indirect = 1;
        xrt_header->completion_signal = comp_sig_dma;

        ctrl_size = 0;
        for (auto &[col, col_code] : cel.ccode.cols) {
            struct common_header *ch;
            struct exec_buf *eb;
            void *ctrl_code_va;
            uint64_t ctrl_code_dma;

            ctrl_code_va = (char *)XAie_MemGetVAddr(ctrl_code_meminst) + ctrl_size;
            ctrl_code_dma = XAie_MemGetDevAddr(ctrl_code_meminst) + ctrl_size;
            memcpy(ctrl_code_va, col_code.code.data(), col_code.code.size());

            indirect->uc_index = col;
            indirect->host_addr_high = (entry_dma >> 32) & 0xFFFFFFFFUL;
            indirect->host_addr_low = entry_dma & ((1 << 25) - 1);

            ch = (struct common_header *)entry_va;
            memset(ch, 0, sizeof(*ch));
            eb = (struct exec_buf *)entry_va + sizeof(*ch);
            memset(eb, 0, sizeof(*eb));

            ch->type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
            ch->opcode = HOST_QUEUE_PACKET_EXEC_BUF;
            ch->count = sizeof(struct exec_buf);
            ch->distribute = 1;

            eb->cu_index = 0;
            eb->dpu_control_code_host_addr_high = (ctrl_code_dma >> 32) & 0xFFFFFFFFUL;
            eb->dpu_control_code_host_addr_low = ctrl_code_dma & 0xFFFFFFFFUL;

            if (this->external_buffer_map.size()) {
                eb->args_host_addr_high = this->host_args_high;
                eb->args_host_addr_low = this->host_args_low;
                eb->args_len = this->external_buffer_map.size() * sizeof(uint64_t);
            }

            indirect++;
            entry_va = (char *)entry_va + sizeof(struct common_header) + sizeof(struct exec_buf);
            entry_dma += sizeof(struct common_header) + sizeof(struct exec_buf);
            ctrl_size += col_code.code.size();
        }
        XAie_MemSyncForDev(ctrl_code_meminst);
        XAie_MemSyncForDev(entry_meminst);
        XAie_MemSyncForDev(this->hsa_meminst);
        write_index = (write_index + 1) & (HSA_QUEUE_ENTRY - 1);
        smp_wmb();
        hq_header->write_index = write_index;
        XAie_MemSyncForDev(this->hsa_meminst);
        hexdump(hq_header, sizeof(*hq_header));
        do {
            XAie_MemSyncForCPU(this->comp_sig_meminst);
            printf("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffff);
        } while (!READ_ONCE(comp_sig_va));
        XAie_MemFree(ctrl_code_meminst);
        XAie_MemFree(entry_meminst);
    } else {
        /* case 4 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        struct host_indirect_packet_entry *indirect;
        struct host_indirect_packet_entry *first_indirect;
        struct common_header *first_ch;
        uint64_t first_indirect_dma;
        XAie_MemInst *entry_meminst;
        char *entry_va;
        uint64_t entry_dma;
        XAie_MemInst *ctrl_code_meminst;
        size_t ctrl_size = 0;
        size_t entry_size;

        entry_size = sizeof(struct common_header) +
                     cel.ccode.cols.size() * (sizeof(struct host_indirect_packet_entry) + sizeof(struct common_header) +
                                              sizeof(struct exec_buf));
        entry_meminst = XAie_MemAllocate(this->DevInst, entry_size, XAIE_MEM_NONCACHEABLE);
        entry_va = (char *)XAie_MemGetVAddr(entry_meminst);
        entry_dma = XAie_MemGetDevAddr(entry_meminst);
        if (!entry_meminst) {
            printf("entries alloc failed.\n");

            return -1;
        }

        for (auto &[col, col_code] : cel.ccode.cols) {
            ctrl_size += col_code.code.size();
        }
        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, ctrl_size, XAIE_MEM_NONCACHEABLE);
        if (!ctrl_code_meminst) {
            printf("ctrl code meminst alloc failed.\n");
            XAie_MemFree(entry_meminst);

            return -1;
        }

        memset(hq_entry, 0, sizeof(*hq_entry));
        xrt_header->common_header.type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
        xrt_header->common_header.opcode = HOST_QUEUE_PACKET_EXEC_BUF;
        xrt_header->common_header.count = sizeof(struct host_indirect_packet_entry);
        xrt_header->common_header.distribute = 1;
        xrt_header->common_header.indirect = 1;
        xrt_header->completion_signal = comp_sig_dma;

        first_indirect = (struct host_indirect_packet_entry *)&hq_entry->data[0];
        first_indirect_dma = entry_dma;
        first_indirect->uc_index = 0;
        first_indirect->host_addr_high = (first_indirect_dma >> 32) & 0xFFFFFFFFUL;
        first_indirect->host_addr_low = first_indirect_dma & 0xFFFFFFFFUL;

        first_ch = (struct common_header *)(entry_va);
        first_ch->type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
        first_ch->opcode = HOST_QUEUE_PACKET_EXEC_BUF;
        first_ch->count = cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry);
        first_ch->distribute = 1;
        first_ch->indirect = 1;

        indirect = (struct host_indirect_packet_entry *)((uint8_t *)first_ch + sizeof(*first_ch));
        entry_va += sizeof(struct common_header) + cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry);
        entry_dma += sizeof(struct common_header) + cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry);

        ctrl_size = 0;
        for (auto &[col, col_code] : cel.ccode.cols) {
            struct common_header *ch;
            struct exec_buf *eb;
            void *ctrl_code_va;
            uint64_t ctrl_code_dma;

            ctrl_code_va = (char *)XAie_MemGetVAddr(ctrl_code_meminst) + ctrl_size;
            ctrl_code_dma = XAie_MemGetDevAddr(ctrl_code_meminst) + ctrl_size;
            memcpy(ctrl_code_va, col_code.code.data(), col_code.code.size());

            indirect->uc_index = col;
            indirect->host_addr_high = (entry_dma >> 32) & 0xFFFFFFFFUL;
            indirect->host_addr_low = entry_dma & ((1 << 25) - 1);

            ch = (struct common_header *)entry_va;
            memset(ch, 0, sizeof(*ch));
            eb = (struct exec_buf *)entry_va + sizeof(*ch);
            memset(eb, 0, sizeof(*eb));

            ch->type = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
            ch->opcode = HOST_QUEUE_PACKET_EXEC_BUF;
            ch->count = sizeof(struct exec_buf);
            ch->distribute = 1;

            eb->cu_index = 0;
            eb->dpu_control_code_host_addr_high = (ctrl_code_dma >> 32) & 0xFFFFFFFFUL;
            eb->dpu_control_code_host_addr_low = ctrl_code_dma & 0xFFFFFFFFUL;

            if (this->external_buffer_map.size()) {
                eb->args_host_addr_high = this->host_args_high;
                eb->args_host_addr_low = this->host_args_low;
                eb->args_len = this->external_buffer_map.size() * sizeof(uint64_t);
            }

            indirect++;
            entry_va = (char *)entry_va + sizeof(struct common_header) + sizeof(struct exec_buf);
            entry_dma += sizeof(struct common_header) + sizeof(struct exec_buf);
            ctrl_size += col_code.code.size();
        }
        XAie_MemSyncForDev(ctrl_code_meminst);
        XAie_MemSyncForDev(entry_meminst);
        XAie_MemSyncForDev(this->hsa_meminst);
        write_index = (write_index + 1) & (HSA_QUEUE_ENTRY - 1);
        smp_wmb();
        hq_header->write_index = write_index;
        XAie_MemSyncForDev(this->hsa_meminst);
        hexdump(hq_header, sizeof(*hq_header));
        do {
            XAie_MemSyncForCPU(this->comp_sig_meminst);
            printf("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffff);
        } while (!READ_ONCE(comp_sig_va));
        XAie_MemFree(ctrl_code_meminst);
        XAie_MemFree(entry_meminst);
    }

    return *comp_sig_va == HSA_COMP_SUCCESS ? 0 : *comp_sig_va;
}

int cert_api::submit_test_command() {
    struct hsa_queue *hsa_q = (struct hsa_queue *)(XAie_MemGetVAddr(this->hsa_meminst));
    struct host_queue_header *hq_header = &hsa_q->hq_header;
    struct host_queue_packet *hq_entry;
    struct xrt_packet_header *xrt_packet_header;
    uint32_t *completion_signal;
    uint64_t completion_signal_dma_addr;
    uint64_t write_index;

    XAie_MemSyncForCPU(this->hsa_meminst);
    write_index = hq_header->write_index;

    completion_signal = (uint32_t *)XAie_MemGetVAddr(this->comp_sig_meminst);
    completion_signal = &completion_signal[write_index];
    XAie_MemSyncForCPU(this->comp_sig_meminst);
    *completion_signal = 0;
    XAie_MemSyncForDev(this->comp_sig_meminst);
    completion_signal_dma_addr = XAie_MemGetDevAddr(this->comp_sig_meminst);
    completion_signal_dma_addr += sizeof(*completion_signal) * write_index;

    hq_entry = &hsa_q->hq_entry[write_index];
    xrt_packet_header = &hq_entry->xrt_header;
    memset(hq_entry, 0, sizeof(*hq_entry));
    xrt_packet_header->common_header.header = HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC;
    xrt_packet_header->common_header.opcode = HOST_QUEUE_PACKET_TEST;
    xrt_packet_header->completion_signal = completion_signal_dma_addr;

    XAie_MemSyncForDev(this->hsa_meminst);
    write_index = (write_index + 1) & (HSA_QUEUE_ENTRY - 1);
    smp_wmb();
    hq_header->write_index = write_index;
    XAie_MemSyncForDev(this->hsa_meminst);

    do {
        XAie_MemSyncForCPU(this->comp_sig_meminst);
        printf("completion signal: %d\n", READ_ONCE(completion_signal));
        this->run_cert(0xfff);
    } while (!READ_ONCE(completion_signal));
    return *completion_signal == HSA_COMP_SUCCESS ? 0 : *completion_signal;
}

} /* namespace cert_host */