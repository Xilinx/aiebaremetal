// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "aeg_log.h"
#include "barrier.h"
#include "cert_host.h"
#include "cert_elf.h"

#include <unistd.h>
#include <cstring>

namespace cert_host {
using mem_barrier::READ_ONCE;
using mem_barrier::smp_wmb;

cert_api::cert_api(XAie_DevInst *DevInst) : DevInst(DevInst) { AEG_LOG("Handshake magic value: 0x%x\n", ALIVE_MAGIC); }

cert_api::cert_api() {}

cert_api::~cert_api() {
    if (this->hsa_meminst != nullptr) {
        (void)XAie_MemFree(this->hsa_meminst);
        this->hsa_meminst = nullptr;
    }
    if (this->comp_sig_meminst != nullptr) {
        (void)XAie_MemFree(this->comp_sig_meminst);
        this->comp_sig_meminst = nullptr;
    }
}

void cert_api::init_devinst(XAie_DevInst *dev_inst) {
    this->DevInst = dev_inst;
    AEG_LOG("Handshake value: 0x%x\n", ALIVE_MAGIC);
}

int32_t cert_api::wakeup_cert(uint32_t col) const {
    AieRC RC = XAIE_OK;
    XAie_LocType Loc = {0, static_cast<std::uint8_t>(col)};
    std::uint8_t tile_type = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[tile_type].UcMod;

    AEG_LOG("Waking up cert on col: %d\n", col);
    RC = UcMod->Wakeup(DevInst, Loc, UcMod);
    if (RC != XAIE_OK) {
        AEG_ERROR("Wakeup failed: %d\n", static_cast<int32_t>(RC));
    }

    return static_cast<int32_t>(RC);
}

int32_t cert_api::wakeup_cert() const {
    const uint32_t start_col = static_cast<uint32_t>(DevInst->StartCol);
    const uint32_t end_col = static_cast<uint32_t>(DevInst->StartCol) + static_cast<uint32_t>(DevInst->NumCols);
    uint32_t ret = 0U;

    for (uint32_t i = start_col; i < end_col; ++i) {
        ret |= static_cast<uint32_t>(wakeup_cert(i));
    }

    return static_cast<int32_t>(ret);
}

int32_t cert_api::load_cert() const {
    int32_t ret = 0;
    const uint32_t start_col_val = static_cast<uint32_t>(DevInst->StartCol);
    const uint32_t end_col_val = start_col_val + static_cast<uint32_t>(DevInst->NumCols);

    for (uint32_t col = start_col_val; col < end_col_val; ++col) {
        ret = load_cert(col);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

int32_t cert_api::load_cert(uint32_t col) const {
    AEG_LOG("Loading cert in col: %d cert_elf: %p cert_elf_len: %d\n", col,
            static_cast<void *>(std::addressof(cert_elf[0])), cert_elf_len);

    AieRC RC = XAIE_OK;
    XAie_LocType Loc = {0, static_cast<uint8_t>(col)};
    std::uint8_t tile_type = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[tile_type].UcMod;
    uint32_t CoreStatus = 0;

    if (cert_elf_len == 0) {
        AEG_ERROR("Cert elf len zero.Cannot load cert.\n");
        return -1;
    }
    RC = UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
    if (RC != XAIE_OK) {
        AEG_ERROR("GetCoreStaute failed: %d\n", static_cast<int32_t>(RC));
    }
    AEG_LOG("[%d]: CoreStatus: %d\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(CoreStatus));

    AEG_LOG("Loading cert in col: %d\n", static_cast<int32_t>(Loc.Col));
    const std::uint8_t *const cert_elf_data = std::addressof(cert_elf[0]);
    RC = XAie_LoadUcMem(DevInst, Loc, cert_elf_data);
    if (RC != XAIE_OK) {
        AEG_ERROR("Load Cert in col %d failed: %d\n", static_cast<int32_t>(Loc.Col),
                  static_cast<int32_t>(RC));
    }
    RC = UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
    if (RC != XAIE_OK) {
        AEG_ERROR("GetCoreStaute failed: %d\n", static_cast<int32_t>(RC));
    }
    AEG_LOG("[%d]: CoreStatus: %d\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(CoreStatus));

    return 0;
}

void cert_api::run_cert(uint32_t cycles) const {
    XAie_LocType Loc = {};
    const uint32_t end_col = static_cast<uint32_t>(DevInst->StartCol) + static_cast<uint32_t>(DevInst->NumCols);

    while (cycles != 0U) {
        --cycles;
        for (uint32_t col_idx = static_cast<uint32_t>(DevInst->StartCol); col_idx < end_col; ++col_idx) {
            Loc.Col = static_cast<uint8_t>(col_idx);
            uint8_t tile_type = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
            const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[tile_type].UcMod;
            uint32_t CoreStatus = 0;

            (void)UcMod->GetCoreStatus(DevInst, Loc, &CoreStatus, UcMod);
        }
    }
}

int32_t cert_api::init_handshake() {
    XAie_LocType Loc = {0, 0};
    AieRC RC = XAIE_OK;
    struct hsa_queue *hsa_q;
    struct hsa_queue *hsa_q_dma;
    uint64_t hsa_q_dma_addr = 0U;
    uint32_t *comp_sig_va = nullptr;
    uint32_t num_cols = 0U;

    this->hsa_meminst = XAie_MemAllocate(this->DevInst, sizeof(struct hsa_queue), XAIE_MEM_NONCACHEABLE);
    if (!this->hsa_meminst) {
        AEG_ERROR("Failed to allocate hsa queue.\n");
        return -1;
    }

    this->comp_sig_meminst = XAie_MemAllocate(this->DevInst, sizeof(uint32_t) * HSA_QUEUE_ENTRY, XAIE_MEM_NONCACHEABLE);
    if (!this->comp_sig_meminst) {
        AEG_ERROR("completion signal alloc failed.\n");
        static_cast<void>(XAie_MemFree(this->hsa_meminst));
        this->hsa_meminst = nullptr;
        return -ENOMEM;
    }

    comp_sig_va = static_cast<uint32_t *>(XAie_MemGetVAddr(this->comp_sig_meminst));
    static_cast<void>(memset(comp_sig_va, 0, sizeof(*comp_sig_va) * static_cast<std::size_t>(HSA_QUEUE_ENTRY)));

    hsa_q = static_cast<struct hsa_queue *>(XAie_MemGetVAddr(this->hsa_meminst));
    static_cast<void>(memset(hsa_q, 0, sizeof(*hsa_q)));
    hsa_q_dma_addr = XAie_MemGetDevAddr(this->hsa_meminst);
    hsa_q_dma = reinterpret_cast<struct hsa_queue *>(static_cast<uintptr_t>(hsa_q_dma_addr));
    hsa_q->hq_header.capacity = HSA_QUEUE_ENTRY;
    hsa_q->hq_header.data_address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&hsa_q_dma->hq_entry[0]));
    static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));

    static_cast<void>(memset(&this->handshake, 0, sizeof(this->handshake)));
    handshake.mpaie_alive = ALIVE_MAGIC;
    {
        const uint32_t start_col_u = static_cast<uint32_t>(DevInst->StartCol);
        const uint32_t col_shift_u = static_cast<uint32_t>(DevInst->DevProp.ColShift);
        handshake.partition_base_address = start_col_u << col_shift_u;
    }
    handshake.aie_info.partition_size = DevInst->NumCols;
    num_cols = static_cast<uint32_t>(this->DevInst->NumCols);
    for (uint32_t col_idx = 0U; col_idx < num_cols; ++col_idx) {
        Loc.Col = static_cast<uint8_t>(col_idx);
        if (col_idx == 0U) {
            handshake.hsa_addr_high =
                static_cast<uint32_t>((hsa_q_dma_addr >> 32U) & static_cast<uint64_t>(UINT32_MAX));
            handshake.hsa_addr_low =
                static_cast<uint32_t>(hsa_q_dma_addr & static_cast<uint64_t>(UINT32_MAX));
        } else {
            handshake.hsa_addr_high = UINT32_MAX;
            handshake.hsa_addr_low = UINT32_MAX;
        }

        RC = XAie_DataMemBlockWrite(DevInst, Loc, static_cast<uint32_t>(0U), &handshake,
                                    static_cast<uint32_t>(sizeof(this->handshake)));
        if (RC != XAIE_OK) {
            AEG_ERROR("Init of handshake failed: %d", static_cast<int32_t>(RC));
        } else {
        }
    }
    this->parse_host_args();

    return 0;
}

void cert_api::parse_host_args() {
    uint32_t *host_args = nullptr;
    uint64_t host_args_dev_addr = 0U;
    int32_t i = 0;
    AieRC RC = XAIE_OK;

    if (this->external_buffer_map.empty()) {
        this->host_args_low = 0;
        this->host_args_high = 0;
        this->host_args_meminst = nullptr;
        return;
    }

    this->host_args_meminst =
        XAie_MemAllocate(this->DevInst, this->external_buffer_map.size() * sizeof(uint64_t), XAIE_MEM_NONCACHEABLE);
    if (!this->host_args_meminst) {
        AEG_ERROR("Failed to allocate mem for host args.\n");
        this->host_args_low = 0;
        this->host_args_high = 0;
        this->host_args_meminst = nullptr;
        return;
    }

    host_args = static_cast<uint32_t *>(XAie_MemGetVAddr(this->host_args_meminst));
    for (const auto &buffer_entry : this->external_buffer_map) {
        const auto &sim_id = buffer_entry.second;
        uint64_t addr = this->external_buffer_addr_map[sim_id];

        /* Addr High */
        host_args[i++] = static_cast<uint32_t>((addr >> 32U) & static_cast<std::uint64_t>(UINT32_MAX));
        /* Addr Low */
        host_args[i++] = static_cast<uint32_t>(addr & static_cast<uint64_t>(UINT32_MAX));
    }

    host_args_dev_addr = XAie_MemGetDevAddr(this->host_args_meminst);
    this->host_args_high = static_cast<uint32_t>((host_args_dev_addr >> 32U) & static_cast<uint64_t>(UINT32_MAX));
    this->host_args_low = static_cast<uint32_t>(host_args_dev_addr & static_cast<uint64_t>(UINT32_MAX));
    RC = XAie_MemSyncForDev(this->host_args_meminst);
    if (RC != XAIE_OK) {
        AEG_ERROR("host args sync for dev failed: %d\n", static_cast<std::int32_t>(RC));
        this->host_args_low = 0;
        this->host_args_high = 0;
        this->host_args_meminst = nullptr;
        return;
    }
}

int32_t cert_api::submit_command(const char *ctrl_elf, uint32_t ctrl_elf_len,
                                      const std::map<std::string_view, uint64_t> &symbol) const {
    cert_elfloader::cert_elf_loader cel(ctrl_elf, ctrl_elf_len, symbol);
    struct hsa_queue *hsa_q = static_cast<struct hsa_queue *>(XAie_MemGetVAddr(this->hsa_meminst));
    struct hsa_queue *hsa_q_dma_addr = reinterpret_cast<struct hsa_queue *>(static_cast<uintptr_t>(XAie_MemGetDevAddr(this->hsa_meminst)));
    static_cast<void>(hsa_q_dma_addr);
    struct host_queue_header *hq_header = &hsa_q->hq_header;
    struct host_queue_packet *hq_entry = nullptr;
    uint32_t *comp_sig_va = nullptr;
    uint64_t comp_sig_dma = 0U;
    uint64_t write_index = 0U;

    static_cast<void>(XAie_MemSyncForCPU(this->hsa_meminst));
    write_index = hq_header->write_index;

    hq_entry = &hsa_q->hq_entry[write_index];

    comp_sig_va = static_cast<uint32_t *>(XAie_MemGetVAddr(this->comp_sig_meminst));
    comp_sig_va = &comp_sig_va[write_index];
    comp_sig_dma = XAie_MemGetDevAddr(this->comp_sig_meminst);
    comp_sig_dma += sizeof(uint32_t) * write_index;
    static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
    *comp_sig_va = 0;
    static_cast<void>(XAie_MemSyncForDev(this->comp_sig_meminst));

    static_cast<void>(this->wakeup_cert());

    static_cast<void>(cel.parse());
    AEG_LOG("ccode.cols.size: %zu\n", cel.ccode.cols.size());
    if (cel.ccode.cols.size() > 0) {
        AEG_LOG("ccode.cols.code.size: %zu\n", cel.ccode.cols[0].code.size());
    }
    if ((cel.ccode.cols.size() > 24) || (cel.ccode.cols.size() == 0)) {
        AEG_ERROR("Cols greater than 24 or 0 not supported\n");
        return -1;
    } else if (cel.ccode.cols.size() == 1) {
        /* Case 1 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        XAie_MemInst *ctrl_code_meminst = nullptr;
        void *ctrl_code = nullptr;
        uint64_t ctrl_code_dma_addr = 0U;
        struct exec_buf *eb_ptr = nullptr;

        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, cel.ccode.cols[0].code.size(), XAIE_MEM_NONCACHEABLE);
        ctrl_code = XAie_MemGetVAddr(ctrl_code_meminst);
        ctrl_code_dma_addr = XAie_MemGetDevAddr(ctrl_code_meminst);
        if (!ctrl_code) {
            AEG_ERROR("mem alloc of ctrl code failed\n");
            return -1;
        }
        static_cast<void>(memcpy(ctrl_code, cel.ccode.cols[0].code.data(), cel.ccode.cols[0].code.size()));
        static_cast<void>(XAie_MemSyncForDev(ctrl_code_meminst));
        AEG_LOG("ctrl_code: 0x%llx size: %d ctrl_code_dma_addr: 0x%llx\n", ctrl_code, cel.ccode.cols[0].code.size(),
                ctrl_code_dma_addr);
        cert_elfloader::hexdump(ctrl_code, static_cast<ssize_t>(cel.ccode.cols[0].code.size()));

        static_cast<void>(memset(hq_entry, 0, sizeof(*hq_entry)));
        xrt_header->common_header.type =
            static_cast<std::uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
        xrt_header->common_header.opcode = static_cast<std::uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
        xrt_header->common_header.count = static_cast<std::uint16_t>(sizeof(struct exec_buf));
        AEG_LOG("cound = %d\n", static_cast<std::int32_t>(xrt_header->common_header.count));
        xrt_header->common_header.distribute = static_cast<std::uint8_t>(0U);
        xrt_header->common_header.indirect = static_cast<std::uint8_t>(0U);
        xrt_header->completion_signal = comp_sig_dma;
        eb_ptr = reinterpret_cast<struct exec_buf *>(reinterpret_cast<char *>(hq_entry) + sizeof(struct xrt_packet_header));
        eb_ptr->cu_index = 0;
        eb_ptr->dpu_control_code_host_addr_high =
            static_cast<uint32_t>((ctrl_code_dma_addr >> 32U) & static_cast<uint64_t>(UINT32_MAX));
        eb_ptr->dpu_control_code_host_addr_low =
            static_cast<uint32_t>(ctrl_code_dma_addr & static_cast<uint64_t>(UINT32_MAX));

            AEG_LOG("hq_entry: 0x%lx\n",
                reinterpret_cast<std::uintptr_t>(&hsa_q_dma_addr->hq_entry[hq_header->write_index]));
        cert_elfloader::hexdump(hq_entry, static_cast<ssize_t>(sizeof(*hq_entry)));
        if (this->external_buffer_map.size()) {
            eb_ptr->args_host_addr_high = this->host_args_high;
            eb_ptr->args_host_addr_low = this->host_args_low;
            eb_ptr->args_len = static_cast<uint16_t>(this->external_buffer_map.size() * sizeof(uint64_t));
        }
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        cert_elfloader::hexdump(hq_header, static_cast<ssize_t>(sizeof(*hq_header)));
        write_index = (write_index + 1ULL) & (static_cast<uint64_t>(HSA_QUEUE_ENTRY) - 1ULL);
        smp_wmb();
        hq_header->write_index = write_index;
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        cert_elfloader::hexdump(hq_header, static_cast<ssize_t>(sizeof(*hq_header)));
        do {
            static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
            AEG_LOG("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffffU);
        } while (READ_ONCE(comp_sig_va) == 0U);
        static_cast<void>(XAie_MemFree(ctrl_code_meminst));
    } else if (cel.ccode.cols.size() <= HSA_MAX_LEVEL1_INDIRECT_ENTRIES) {
        /* case 3 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        struct host_indirect_packet_entry *indirect = nullptr;
        XAie_MemInst *entry_meminst = nullptr;
        char *entry_va = nullptr;
        uint64_t entry_dma = 0U;
        XAie_MemInst *ctrl_code_meminst = nullptr;
        size_t ctrl_size = 0;

        indirect = static_cast<struct host_indirect_packet_entry *>(static_cast<void *>(&hq_entry->data[0]));

        entry_meminst = XAie_MemAllocate(
            this->DevInst,
            (cel.ccode.cols.size() * (sizeof(struct common_header) + sizeof(struct exec_buf))),
            XAIE_MEM_NONCACHEABLE);
        entry_va = static_cast<char *>(XAie_MemGetVAddr(entry_meminst));
        entry_dma = XAie_MemGetDevAddr(entry_meminst);
        if (!entry_meminst) {
            AEG_ERROR("entries alloc failed.\n");

            return -1;
        }

        for (auto &[col, column_code] : cel.ccode.cols) {
            ctrl_size += column_code.code.size();
        }
        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, ctrl_size, XAIE_MEM_NONCACHEABLE);
        if (!ctrl_code_meminst) {
            AEG_ERROR("ctrl code meminst alloc failed.\n");
            static_cast<void>(XAie_MemFree(entry_meminst));

            return -1;
        }

        static_cast<void>(memset(hq_entry, 0, sizeof(*hq_entry)));
        xrt_header->common_header.type =
            static_cast<std::uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
        xrt_header->common_header.opcode = static_cast<uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
        xrt_header->common_header.count = static_cast<uint16_t>(cel.ccode.cols.size() *
                                                                     sizeof(struct host_indirect_packet_entry));
        xrt_header->common_header.distribute = static_cast<uint8_t>(1U);
        xrt_header->common_header.indirect = static_cast<uint8_t>(1U);
        xrt_header->completion_signal = comp_sig_dma;

        ctrl_size = 0;
        for (auto &[col, column_code] : cel.ccode.cols) {
            struct common_header *ch = nullptr;
            struct exec_buf *eb = nullptr;
            void *ctrl_code_va = nullptr;
            uint64_t ctrl_code_dma = 0U;

            ctrl_code_va = static_cast<char *>(XAie_MemGetVAddr(ctrl_code_meminst)) + ctrl_size;
            ctrl_code_dma = XAie_MemGetDevAddr(ctrl_code_meminst) + ctrl_size;
            static_cast<void>(memcpy(ctrl_code_va, column_code.code.data(), column_code.code.size()));

            indirect->uc_index = static_cast<uint32_t>(col);
            indirect->host_addr_high = static_cast<uint32_t>((entry_dma >> 32U) & static_cast<uint64_t>(UINT32_MAX));
            indirect->host_addr_low = static_cast<uint32_t>(entry_dma & static_cast<uint64_t>((1ULL << 25) - 1ULL));

            ch = static_cast<struct common_header *>(static_cast<void *>(entry_va));
            static_cast<void>(memset(ch, 0, sizeof(*ch)));
            eb = static_cast<struct exec_buf *>(static_cast<void *>(entry_va + sizeof(*ch)));
            static_cast<void>(memset(eb, 0, sizeof(*eb)));

            ch->type = static_cast<uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
            ch->opcode = static_cast<uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
            ch->count = static_cast<uint16_t>(sizeof(struct exec_buf));
            ch->distribute = static_cast<uint8_t>(1U);

            eb->cu_index = 0;
            eb->dpu_control_code_host_addr_high = static_cast<uint32_t>((ctrl_code_dma >> 32U) & static_cast<uint64_t>(UINT32_MAX));
            eb->dpu_control_code_host_addr_low = static_cast<uint32_t>(ctrl_code_dma & static_cast<uint64_t>(UINT32_MAX));

            if (this->external_buffer_map.size()) {
                eb->args_host_addr_high = this->host_args_high;
                eb->args_host_addr_low = this->host_args_low;
                eb->args_len =
                    static_cast<uint16_t>(this->external_buffer_map.size() * sizeof(uint64_t));
            }

            indirect++;
            entry_va = entry_va + sizeof(struct common_header) + sizeof(struct exec_buf);
            entry_dma += sizeof(struct common_header) + sizeof(struct exec_buf);
            ctrl_size += column_code.code.size();
        }
        static_cast<void>(XAie_MemSyncForDev(ctrl_code_meminst));
        static_cast<void>(XAie_MemSyncForDev(entry_meminst));
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        write_index = (write_index + 1ULL) & (static_cast<uint64_t>(HSA_QUEUE_ENTRY) - 1ULL);
        smp_wmb();
        hq_header->write_index = write_index;
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        cert_elfloader::hexdump(hq_header, static_cast<ssize_t>(sizeof(*hq_header)));
        do {
            static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
            AEG_LOG("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffffU);
        } while (READ_ONCE(comp_sig_va) == 0U);
        static_cast<void>(XAie_MemFree(ctrl_code_meminst));
        static_cast<void>(XAie_MemFree(entry_meminst));
    } else {
        /* case 4 */
        struct xrt_packet_header *xrt_header = &hq_entry->xrt_header;
        struct host_indirect_packet_entry *indirect = nullptr;
        struct host_indirect_packet_entry *first_indirect = nullptr;
        struct common_header *first_ch = nullptr;
        uint64_t first_indirect_dma = 0U;
        XAie_MemInst *entry_meminst = nullptr;
        char *entry_va = nullptr;
        uint64_t entry_dma = 0U;
        XAie_MemInst *ctrl_code_meminst = nullptr;
        size_t ctrl_size = 0;
        std::size_t entry_size = 0;

        entry_size = sizeof(struct common_header) +
                     (cel.ccode.cols.size() *
                      (sizeof(struct host_indirect_packet_entry) + sizeof(struct common_header) + sizeof(struct exec_buf)));
        entry_meminst = XAie_MemAllocate(this->DevInst, entry_size, XAIE_MEM_NONCACHEABLE);
        entry_va = static_cast<char *>(XAie_MemGetVAddr(entry_meminst));
        entry_dma = XAie_MemGetDevAddr(entry_meminst);
        if (!entry_meminst) {
            AEG_ERROR("entries alloc failed.\n");

            return -1;
        }

        for (auto &[col, column_code] : cel.ccode.cols) {
            ctrl_size += column_code.code.size();
        }
        ctrl_code_meminst = XAie_MemAllocate(this->DevInst, ctrl_size, XAIE_MEM_NONCACHEABLE);
        if (!ctrl_code_meminst) {
            AEG_ERROR("ctrl code meminst alloc failed.\n");
            static_cast<void>(XAie_MemFree(entry_meminst));

            return -1;
        }

        static_cast<void>(memset(hq_entry, 0, sizeof(*hq_entry)));
        xrt_header->common_header.type =
            static_cast<std::uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
        xrt_header->common_header.opcode = static_cast<uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
        xrt_header->common_header.count = static_cast<uint16_t>(sizeof(struct host_indirect_packet_entry));
        xrt_header->common_header.distribute = static_cast<uint8_t>(1U);
        xrt_header->common_header.indirect = static_cast<uint8_t>(1U);
        xrt_header->completion_signal = comp_sig_dma;

        first_indirect = static_cast<struct host_indirect_packet_entry *>(static_cast<void *>(&hq_entry->data[0]));
        first_indirect_dma = entry_dma;
        first_indirect->uc_index = 0;
        first_indirect->host_addr_high = static_cast<uint32_t>((first_indirect_dma >> 32U) & static_cast<uint64_t>(UINT32_MAX));
        first_indirect->host_addr_low = static_cast<uint32_t>(first_indirect_dma & static_cast<uint64_t>(UINT32_MAX));

        first_ch = static_cast<struct common_header *>(static_cast<void *>(entry_va));
        first_ch->type = static_cast<uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
        first_ch->opcode = static_cast<uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
        first_ch->count = static_cast<uint16_t>(cel.ccode.cols.size() *
                                                      sizeof(struct host_indirect_packet_entry));
        first_ch->distribute = static_cast<uint8_t>(1U);
        first_ch->indirect = static_cast<uint8_t>(1U);

        indirect = static_cast<struct host_indirect_packet_entry *>(
            static_cast<void *>(static_cast<char *>(static_cast<void *>(first_ch)) + sizeof(*first_ch)));
        entry_va += sizeof(struct common_header) + (cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry));
        entry_dma += sizeof(struct common_header) + (cel.ccode.cols.size() * sizeof(struct host_indirect_packet_entry));

        ctrl_size = 0;
        for (auto &[col, column_code] : cel.ccode.cols) {
            struct common_header *ch = nullptr;
            struct exec_buf *eb = nullptr;
            void *ctrl_code_va = nullptr;
            uint64_t ctrl_code_dma = 0U;

            ctrl_code_va = static_cast<char *>(XAie_MemGetVAddr(ctrl_code_meminst)) + ctrl_size;
            ctrl_code_dma = XAie_MemGetDevAddr(ctrl_code_meminst) + ctrl_size;
            static_cast<void>(memcpy(ctrl_code_va, column_code.code.data(), column_code.code.size()));

            indirect->uc_index = static_cast<uint32_t>(col);
            indirect->host_addr_high = static_cast<uint32_t>((entry_dma >> 32U) & static_cast<uint64_t>(UINT32_MAX));
            indirect->host_addr_low =
                static_cast<uint32_t>(entry_dma & static_cast<uint64_t>((1ULL << 25) - 1ULL));

            ch = static_cast<struct common_header *>(static_cast<void *>(entry_va));
            static_cast<void>(memset(ch, 0, sizeof(*ch)));
            eb = static_cast<struct exec_buf *>(static_cast<void *>(static_cast<char *>(static_cast<void *>(entry_va)) + sizeof(*ch)));
            static_cast<void>(memset(eb, 0, sizeof(*eb)));

            ch->type = static_cast<uint16_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
            ch->opcode = static_cast<uint16_t>(HOST_QUEUE_PACKET_EXEC_BUF);
            ch->count = static_cast<uint16_t>(sizeof(struct exec_buf));
            ch->distribute = static_cast<uint8_t>(1U);

            eb->cu_index = 0;
            eb->dpu_control_code_host_addr_high = static_cast<uint32_t>(ctrl_code_dma >> 32U) & static_cast<uint64_t>(UINT32_MAX);
            eb->dpu_control_code_host_addr_low = static_cast<uint32_t>(ctrl_code_dma & static_cast<uint64_t>(UINT32_MAX));

            if (this->external_buffer_map.size()) {
                eb->args_host_addr_high = this->host_args_high;
                eb->args_host_addr_low = this->host_args_low;
                eb->args_len =
                    static_cast<uint16_t>(this->external_buffer_map.size() * sizeof(uint64_t));
            }

            indirect++;
            entry_va = entry_va + sizeof(struct common_header) + sizeof(struct exec_buf);
            entry_dma += sizeof(struct common_header) + sizeof(struct exec_buf);
            ctrl_size += column_code.code.size();
        }
        static_cast<void>(XAie_MemSyncForDev(ctrl_code_meminst));
        static_cast<void>(XAie_MemSyncForDev(entry_meminst));
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        write_index = (write_index + 1ULL) & (static_cast<uint64_t>(HSA_QUEUE_ENTRY) - 1ULL);
        smp_wmb();
        hq_header->write_index = write_index;
        static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
        cert_elfloader::hexdump(hq_header, static_cast<ssize_t>(sizeof(*hq_header)));
        do {
            static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
            AEG_LOG("completion signal: %d\n", READ_ONCE(comp_sig_va));
            this->run_cert(0xffffffU);
        } while (READ_ONCE(comp_sig_va) == 0U);
        static_cast<void>(XAie_MemFree(ctrl_code_meminst));
        static_cast<void>(XAie_MemFree(entry_meminst));
    }

    return ((*comp_sig_va == HSA_COMP_SUCCESS) ? 0 : static_cast<int32_t>(*comp_sig_va));
}

int32_t cert_api::submit_test_command() const {
    struct hsa_queue *hsa_q = static_cast<struct hsa_queue *>(XAie_MemGetVAddr(this->hsa_meminst));
    struct host_queue_header *hq_header = &hsa_q->hq_header;
    struct host_queue_packet *hq_entry;
    struct xrt_packet_header *xrt_pkt_header;
    uint32_t *completion_signal = nullptr;
    uint64_t completion_signal_dma_addr = 0U;
    uint64_t write_index = 0U;

    static_cast<void>(XAie_MemSyncForCPU(this->hsa_meminst));
    write_index = hq_header->write_index;

    completion_signal = static_cast<uint32_t *>(XAie_MemGetVAddr(this->comp_sig_meminst));
    completion_signal = &completion_signal[write_index];
    static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
    *completion_signal = 0;
    static_cast<void>(XAie_MemSyncForDev(this->comp_sig_meminst));
    completion_signal_dma_addr = XAie_MemGetDevAddr(this->comp_sig_meminst);
    completion_signal_dma_addr += sizeof(*completion_signal) * write_index;

    hq_entry = &hsa_q->hq_entry[write_index];
    xrt_pkt_header = &hq_entry->xrt_header;
    static_cast<void>(memset(hq_entry, 0, sizeof(*hq_entry)));
    xrt_pkt_header->common_header.header = static_cast<uint32_t>(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC);
    xrt_pkt_header->common_header.opcode = static_cast<uint32_t>(HOST_QUEUE_PACKET_TEST);
    xrt_pkt_header->completion_signal = completion_signal_dma_addr;

    static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));
    write_index = (write_index + 1U) & (static_cast<uint64_t>(HSA_QUEUE_ENTRY) - 1U);
    smp_wmb();
    hq_header->write_index = write_index;
    static_cast<void>(XAie_MemSyncForDev(this->hsa_meminst));

    do {
        static_cast<void>(XAie_MemSyncForCPU(this->comp_sig_meminst));
        AEG_LOG("completion signal: %d\n", READ_ONCE(completion_signal));
        this->run_cert(0xfffU);
    } while (READ_ONCE(completion_signal) == 0U);
    return (*completion_signal == static_cast<uint32_t>(HSA_COMP_SUCCESS)) ? 0 : static_cast<int32_t>(*completion_signal);
}

} /* namespace cert_host */