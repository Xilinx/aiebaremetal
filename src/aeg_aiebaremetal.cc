/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */
#include <cstdio>
#include <array>
#include <cinttypes>
#include <unordered_map>
#include <vector>
#ifdef DEBUG_RUNTIME
#include "hw_config.h"
#endif
#ifdef ARMR5
#define AEG_TARGET_ARMR5 1
#endif
// include mpu header (baremetal RPU only; not simulation/Linux/ADF frontends).
// NB: ARMR5 may be defined transitively by xparameters_ps.h, so the include
// guard here must match the guard around the Xil_SetMPURegion() call site.
#if defined(AEG_TARGET_ARMR5) && !defined(__AIESIM__) && !defined(__ADF_FRONTEND__) && !defined(__ZEPHYR__) && !defined(__AIELINUX__)
// Sample ARMR5 before any BSP header is pulled in: xparameters_ps.h (reached
// below via aeg_cdo.h -> xil_io.h) defines ARMR5 on every non-aarch64 target,
// including the x86 sim build. Only the -DARMR5 on the command line means the
// target really is an R5, so test AEG_TARGET_ARMR5 from here on.
#define AEG_HAS_XIL_MPU 1
#include "xil_mpu.h"
#endif
#include "aeg_log.h"
#include <xaiengine.h>
#include "aiebaremetal.h"
#include "aeg_class_api.h"
#include "aeg_cdo.h"
#include "aeg_configure.h"
#include "cert_elf_loader.h"
#include "cert_host.h"
#include "event.h"
#include "log/klogr.h"
#include "xaie_generate_events.h"
#include <xaiengine/xaie_interrupt.h>
#include "xaiemem.h"
#include <unistd.h>
#ifdef __AIESIM__
#include "aeg_ioutils.h"
#include "aiesim_base_apis.h"
#include <dlfcn.h>
#endif

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xparameters_ps.h" /* defines XPAR values */
#include "xscugic.h"
#include <xil_exception.h>
#include <xstatus.h>
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__AIELINUX__)) */
#include "common_layer/aeg_api_message.h"
#include "common_layer/aegothers/fal_util.h"

#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/devicetree.h>
// Map the AIE register aperture at runtime (no SoC MMU-table patch), keyed off
// the xlnx,aiengine DT node (script/aiengine*.overlay). Direct (VA==phys) map so
// it stays out of the dynamic VA allocator (which collides with identity-mapped
// DDR/peripherals -> -EBUSY) and uses 1 GiB blocks; K_MEM_CACHE_NONE gives the
// MT_DEVICE_nGnRnE attrs. Needs CONFIG_KERNEL_DIRECT_MAP + 48-bit VA/PA
// Mapped once, cached. The xlnx,aiengine DT node is referenced directly via
// DT_INST(0, xlnx_aiengine) (a Zephyr devicetree node identifier).
#if !DT_NODE_HAS_STATUS(DT_INST(0, xlnx_aiengine), okay)
#error "aiebaremetal Zephyr build needs an xlnx,aiengine DT node (script/aiengine*.overlay)"
#endif
static uint64_t g_aie_base_va = 0;
static uint64_t abr_map_aie_aperture(void) {
    if (g_aie_base_va == 0U) {
#if defined(CONFIG_MMU)
        uint8_t *va = NULL;
        k_mem_map_phys_bare(&va, (uintptr_t)DT_REG_ADDR(DT_INST(0, xlnx_aiengine)),
                            (size_t)DT_REG_SIZE(DT_INST(0, xlnx_aiengine)),
                            K_MEM_CACHE_NONE | K_MEM_PERM_RW | K_MEM_DIRECT_MAP);
        g_aie_base_va = (uint64_t)(uintptr_t)va;
#else
        // No MMU addressing is flat, use the physical base.
        g_aie_base_va = (uint64_t)DT_REG_ADDR(DT_INST(0, xlnx_aiengine));
#endif
        AEG_LOG("AIE aperture mapped: phys 0x%lx size 0x%lx -> va 0x%lx\n",
                static_cast<std::uint64_t>(DT_REG_ADDR(DT_INST(0, xlnx_aiengine))),
                static_cast<std::uint64_t>(DT_REG_SIZE(DT_INST(0, xlnx_aiengine))),
                static_cast<std::uint64_t>(g_aie_base_va));
    }
    return g_aie_base_va;
}
// For transactionsubmit.h to rebase its AIE base onto the same mapping.
extern "C" uint64_t abr_aie_base_va(void) { return g_aie_base_va; }
#endif /* __ZEPHYR__ */

#ifdef __AIESIM__
#include <adf.h>
#endif

namespace {
inline aeg_err err_from_abr(abr::err_code code) {
    switch (code) {
    case abr::err_code::ok:
        return aeg_err::ok;
    case abr::err_code::user_error:
        return aeg_err::user_error;
    case abr::err_code::internal_error:
        return aeg_err::internal_error;
    case abr::err_code::aie_driver_error:
        return aeg_err::aie_driver_error;
    case abr::err_code::resource_unavailable:
        return aeg_err::resource_unavailable;
    default:
        return aeg_err::aie_driver_error;
    }
}

inline bool is_valid_cstr(const char* value) {
    return (value != nullptr) && (value[0] != '\0');
}

inline bool is_valid_profiling_option(int32_t option) {
    return (option >= static_cast<int32_t>(io_profiling_option::io_total_stream_running_to_idle_cycles)) &&
           (option <= static_cast<int32_t>(io_profiling_option::io_stream_running_event_count));
}
} // namespace
#ifdef __AIESIM__
extern int32_t g_start_plio;
#endif

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
extern "C" {
XScuGic xInterruptController;
}
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__AIELINUX__)) */

namespace {
int32_t get_aie_irq_id();
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
void xscugic_irq_handler(void *data) {
    XScuGic_InterruptHandler(static_cast<XScuGic *>(data));
}
int32_t init_isr = 0;
#endif
} // namespace

void abr_error_isr(void *data);

/// @brief Initializes the AIE error interrupt service routine.
/// Sets up the interrupt controller and registers the error ISR handler.
/// This function is called automatically when the first graph is created.
/// @note This function is only active on hardware (not in simulation or frontend mode).
///       It uses XScuGic interrupt controller and registers handler for AIE IRQ vector.
void abr_init_isr() {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
    int32_t Ret = 0;
    XScuGic_Config *IntcConfig;
    int32_t IrqId = get_aie_irq_id();

    if (init_isr != 0) {
        return;
    }
    Xil_ExceptionInit();
    static_cast<void>(XScuGic_DeviceInitialize(INTC_DEVICE_ID));

    Xil_ExceptionDisable(); /* Initialize the interrupt controller driver */
    IntcConfig = XScuGic_LookupConfig(static_cast<u16>(INTC_DEVICE_ID));
    if (nullptr == IntcConfig) {
        AEG_ERROR("%s:%d: XScuGic_LookupConfig failed\n", "abr_init_isr", __LINE__);
        return;
    }
    Ret = XScuGic_CfgInitialize(&xInterruptController, IntcConfig,
                                static_cast<u32>(IntcConfig->CpuBaseAddress));
    if (Ret != 0) {
        AEG_ERROR("%s:%d: XScuGic_CfgInitialize failed with code %d\n", "abr_init_isr", __LINE__, Ret);
        return;
    }
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
                                 xscugic_irq_handler,
                                 static_cast<void *>(&xInterruptController));

    Ret = static_cast<int32_t>(XScuGic_Connect(&xInterruptController, static_cast<u32>(IrqId),
                                              abr_error_isr,
                                              static_cast<void *>(nullptr)));
    if (Ret != 0) {
        AEG_ERROR("%s:%d: XScuGic_Connect failed with code %d\n", "abr_init_isr", __LINE__, Ret);
        return;
    }

    XScuGic_Enable(&xInterruptController, static_cast<u32>(IrqId));
    Xil_ExceptionEnable();

    init_isr = 1;
    return;
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__) || defined(__AIELINUX__)) */
}

/// @brief Dumps the UC (User Core) application ELF to a file.
/// Writes the embedded UC application ELF binary data to "output.bin" file.
/// This function is only active in AIE simulation mode (excluding the standalone
/// libaiebaremetal_ipc.so build, which does not link the embedded app ELF).
/// @note This is a debug utility function used for extracting the embedded ELF binary.
void dump_uc_app() {
#if defined(__AIESIM__) && !defined(__BAREMETAL_IPC__)
    std::ofstream outFile("output.bin", std::ios::binary | std::ios::out);
    if (!outFile) {
        AEG_ERROR("Failed to open file for writing.");
        return;
    }
    // Read data from istringstream and write it to the binary file
    outFile.write(binary_app_elf_start, binary_app_elf_end - binary_app_elf_start);
    // Close the file
    outFile.close();
#endif
}

/// @brief Test function for AIE driver functionality.
/// Performs various tests on the AIE driver including tile operations,
/// memory allocation, and kernel loading. Currently most test code is commented out.
/// @param DevInst Device instance pointer
/// @note This function is only active in AIE simulation mode.
///       Most test functionality is disabled for embedded software integration.
extern "C" void aie_driver_test(const XAie_DevInst *DevInst) {
    (void)DevInst;
}

/// @brief AIE Graph simulation class for managing graph execution.
/// This class provides the core functionality for initializing, configuring,
/// and executing AIE graphs. It manages device instances, graph APIs, event APIs,
/// and memory allocation for AIE operations.
class AIEGraphSim {
  public:
    /// @brief Destructor for AIEGraphSim.
    virtual ~AIEGraphSim() {
        static_cast<void>(XAie_Finish(&DevInst));
#ifdef __AIESIM__
        using GetFdFn  = int (*)(void);
        using DisconFn = void (*)(int);
        auto get_fd_fn = reinterpret_cast<GetFdFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_backend_get_fd"));
        auto discon_fn = reinterpret_cast<DisconFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_backend_disconnect"));
        if (get_fd_fn && discon_fn && get_fd_fn() >= 0){
            discon_fn(0);
        }
#endif
    }



    AIEGraphSim(const AIEGraphSim &) = delete;
    AIEGraphSim &operator=(const AIEGraphSim &) = delete;
    AIEGraphSim(AIEGraphSim &&) = delete;
    AIEGraphSim &operator=(AIEGraphSim &&) = delete;
    /// @brief Constructor for AIEGraphSim.
    /// Initializes the graph simulation by setting up the device instance,
    /// loading configurations, initializing graph and event APIs, and configuring
    /// the graph. In simulation mode, also opens dynamic libraries for PLIO support.
    /// @param GraphName Name of the graph to initialize
    /// @note The constructor:
    ///       - Sets up device configuration and partition
    ///       - Initializes FAL utilities and config manager
    ///       - Configures the graph
    ///       - Opens simulation libraries if in AIESIM mode
    explicit AIEGraphSim(std::string_view GraphName) {
        graphName = GraphName;
        // RPU (Cortex-R52, AArch32) sees the AIE array via a low 32-bit alias
        // and must override base_address; APU (AArch64) uses the full base, so
        // apply the override only on AArch32.
        struct abr::aiecompiler_options aiecompiler_options{};

        auto &aiecompilerconfig = cfg.get_aiecompiler_config();
        aiecompiler_options.broadcast_enable_core = aiecompilerconfig.broadcast_enable_core;
        AEG_LOG("**********Broadcast enable is %d\n", (aiecompiler_options.broadcast_enable_core ? 1 : 0));
#ifdef __AIESIM__
        using AllocHostFn = void *(*)(uint32_t);
        using FreeHostFn  = void (*)(void *);
        using DevAddrFn   = uint64_t (*)(const void *);
        using WriteFn     = void (*)(uint64_t, const void *, size_t);
        using ReadFn      = void (*)(uint64_t, void *, size_t);
        auto alloc_fn = reinterpret_cast<AllocHostFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_alloc_gm_host"));
        auto free_fn  = reinterpret_cast<FreeHostFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_free_gm_host"));
        auto devaddr_fn = reinterpret_cast<DevAddrFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_dev_addr"));
        auto write_fn = reinterpret_cast<WriteFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_write_gm"));
        auto read_fn  = reinterpret_cast<ReadFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_read_gm"));
        if (!alloc_fn || !free_fn || !devaddr_fn || !write_fn || !read_fn) {
            AEG_ERROR("%s: IPC GM helpers not found via dlsym; the AIESIM IPC backend is required",
                      "AIEGraphSim");
            return;
        }
        AEG_LOG("AIESIM: using IPC-backed XAieMem\n");
        xaiemem = std::make_shared<xaiemem::XAieMem>(alloc_fn, free_fn, devaddr_fn, write_fn, read_fn);

        using GetFdFn   = int (*)(void);
        using ConnectFn = int (*)(const char *);
        auto get_fd_fn  = reinterpret_cast<GetFdFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_backend_get_fd"));
        auto connect_fn = reinterpret_cast<ConnectFn>(dlsym(RTLD_DEFAULT, "aeg_ipc_backend_connect"));
        if (get_fd_fn && connect_fn && get_fd_fn() < 0) {
            const char *socket_path = std::getenv("AEG_PS_IPC_SOCKET");
            if (socket_path && socket_path[0]) {
                AEG_LOG("AIESIM: auto-connecting to %s\n", socket_path);
                connect_fn(socket_path);
            }
        }
#else
        xaiemem = std::make_shared<xaiemem::XAieMem>(&DevInst);
#endif
        auto &dconfig = cfg.get_driver_config();
        auto *gconfig = cfg.get_graph_config(GraphName);
        if (gconfig == nullptr) {
            AEG_ERROR("%s: graph config for '%.*s' not found", "AIEGraphSim", static_cast<int32_t>(GraphName.size()), GraphName.data());
            return;
        }
        GraphApi = std::make_shared<abr::graph_api>(gconfig);
        EventApi = std::make_shared<abr::event>(&cfg);
        auto aiebase = dconfig.base_address;
        // TODO: AIE compiler should provide a rpu base_address, after that change the logic, now just hardcode.
#if defined(AEG_TARGET_ARMR5) || (defined(__ZEPHYR__) && !defined(__aarch64__))
        aiebase = RPU_AIE_BASE;
        AEG_LOG("RPU aiebase = 0x%llx\n", static_cast<int64_t>(aiebase));
#endif
#if defined(__ZEPHYR__)
        // Rebase the config base onto the runtime aperture mapping (see
        // abr_map_aie_aperture); offset handles any base != DT reg base.
        aiebase = abr_map_aie_aperture() +
                  (aiebase - static_cast<uint64_t>(DT_REG_ADDR(DT_INST(0, xlnx_aiengine))));
        AEG_LOG("Zephyr AIE aiebase rebased to va 0x%" PRIx64 "\n",
                static_cast<std::uint64_t>(aiebase));
#endif
        XAie_Config ConfigPtr;
        ConfigPtr.AieGen = dconfig.hw_gen;
        ConfigPtr.BaseAddr = aiebase; // dconfig.base_address
        ConfigPtr.ColShift = dconfig.column_shift;
        ConfigPtr.RowShift = dconfig.row_shift;
        ConfigPtr.NumRows = dconfig.num_rows;
        ConfigPtr.NumCols = dconfig.num_columns;
        ConfigPtr.ShimRowNum = dconfig.shim_row;
        ConfigPtr.MemTileRowStart = dconfig.mem_row_start;
        ConfigPtr.MemTileNumRows = dconfig.mem_num_rows;
        ConfigPtr.AieTileRowStart = dconfig.aie_tile_row_start;
        ConfigPtr.AieTileNumRows = dconfig.aie_tile_num_rows;
        ConfigPtr.PartProp = {0};

        AEG_LOG("dconfig.hw_gen = %d nrows = %d ncol = %d\n", static_cast<int32_t>(dconfig.hw_gen),
                static_cast<int32_t>(dconfig.num_rows), static_cast<int32_t>(dconfig.num_columns));

        AEG_LOG("Before setuppartitionconfig() base address :0x%llx\n", static_cast<int64_t>(DevInst.BaseAddr));
        const uint64_t overlay_col =
            static_cast<uint64_t>(static_cast<uint16_t>(dconfig.partition_overlay_start_cols[0UL]));
        const uint32_t col_shift = static_cast<uint32_t>(dconfig.column_shift);
        AieRC RC = XAie_SetupPartitionConfig(
            &DevInst, (aiebase + (overlay_col << col_shift)),
            static_cast<u8>(dconfig.partition_overlay_start_cols[0UL]), static_cast<u8>(dconfig.partition_num_cols));

        AEG_LOG("After setuppartitionconfig() base address :0x%llx\n", static_cast<std::int64_t>(DevInst.BaseAddr));

        if (RC != XAIE_OK) {
            AEG_ERROR("Partition initialization failed.");
            return;
        }

        RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
        if (RC != XAIE_OK) {
            AEG_ERROR("Driver initialization failed.");
            return;
        }
#if defined(__AIELINUX__)
        RC = XAie_PartitionInitialize(&DevInst, NULL);
        if (RC != XAIE_OK) {
            AEG_ERROR("Partition initialization failed.");
            return;
        }
#endif
        if (!fal::fal_util::initialize(&DevInst)) {
            AEG_ERROR("FAL utility initialization failed.");
        }
        static_cast<void>(abr::config_manager::initialize(&DevInst, static_cast<size_t>(dconfig.mem_num_rows), aiecompiler_options.broadcast_enable_core));
        this->cert.init_devinst(&this->DevInst);
        if (GraphApi != nullptr) {
            static_cast<void>(GraphApi->configure());
        }
    }

    /// @brief Performs self-test and exits.
    /// Tests various functions including error handling, clock enable/disable operations.
    /// In simulation mode, updates NPI address based on device generation.
    /// @note This function is used for debugging and validation purposes.
    ///       It tests error handling initialization and clock control functions.
    aeg_err selftestexit() {
        aeg_err status = aeg_err::ok;

#if defined(__AIESIM__) || defined(__ZEPHYR__)
        AEG_LOG("aie_driver_test pid = %d\n", getpid());
        switch (DevInst.DevProp.DevGen) {
        case XAIE_DEV_GEN_AIE2PS:
            XAie_UpdateNpiAddr(&DevInst, 0xf6d50000U);
            break;
        case XAIE_DEV_GEN_AIEML:
            XAie_UpdateNpiAddr(&DevInst, 0xF6D10000U);
            break;
        case XAIE_DEV_GEN_AIE:
            XAie_UpdateNpiAddr(&DevInst, 0xF70A0000U);
            break;
        default:
            AEG_ERROR("DevGen not found: %d\n", DevInst.DevProp.DevGen);
        }
#endif /* __AIESIM__ */
#ifdef __AIESIM__
        AEG_LOG("Clock enabled.\n");
        AEG_LOG("Clock disabled.\n");
        AEG_LOG("Clock enabled.\n");
#else
        status = clock_enable();
        if (status != aeg_err::ok) {
            AEG_ERROR("Clock enable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", "selftestexit", static_cast<int32_t>(status));
            return status;
        } else {
            AEG_LOG("Clock enabled.\n");
        }
        status = clock_disable();
        if (status != aeg_err::ok) {
            AEG_ERROR("Clock disable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", "selftestexit", static_cast<int32_t>(status));
            return status;
        } else {
            AEG_LOG("Clock disabled.\n");
        }
        status = clock_enable();
        if (status != aeg_err::ok) {
            AEG_ERROR("Clock enable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", "selftestexit", static_cast<int32_t>(status));
            return status;
        } else {
            AEG_LOG("Clock enabled.\n");
        }
#endif /* __AIESIM__ */
        AEG_LOG("selftestexit() completed successfully.\n");
        return aeg_err::ok;
    }

    /// @brief Enables clock for AIE columns.
    /// Enables the clock for all columns in the device partition.
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err clock_enable() {
        AieRC rc = XAie_PmSetColumnClk(&DevInst, static_cast<u32>(DevInst.StartCol), static_cast<u32>(DevInst.NumCols), static_cast<u8>(1));
        if (rc != XAIE_OK) {
            AEG_ERROR("%s: XAie_PmSetColumnClk enable failed rc=%d", "clock_enable", static_cast<int32_t>(rc));
            return aeg_err::aie_driver_error;
        }
        return aeg_err::ok;
    }

    /// @brief Disables clock for AIE columns.
    /// Disables the clock for all columns in the device partition.
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err clock_disable() {
        AieRC rc = XAie_PmSetColumnClk(&DevInst, static_cast<u32>(DevInst.StartCol), static_cast<u32>(DevInst.NumCols), static_cast<u8>(0));
        if (rc != XAIE_OK) {
            AEG_ERROR("%s: XAie_PmSetColumnClk disable failed rc=%d", "clock_disable", static_cast<int32_t>(rc));
            return aeg_err::aie_driver_error;
        }
        return aeg_err::ok;
    }

    /// @brief Sets up callback function for error handling.
    /// Registers a callback function that will be called when errors are detected
    /// during graph execution. The callback receives error payloads and private data.
    /// @param cb Pointer to the callback function
    /// @param priv Private data pointer passed to the callback function
    aeg_err error_handling_cb_init(void *cb, void *priv) {
        error_handling_cb = reinterpret_cast<void (*)(std::vector<XAie_ErrorPayload>, void*)>(cb);
        error_handling_cb_priv = priv;
        return aeg_err::ok;
    }

    /// @brief Initializes the graph and configures GMIOs and external buffers.
    /// Sets up GMIOs and external buffers for data transfer operations.
    /// On ARMR5, also configures MPU region for RPU cache support.
    /// @note This function must be called before running the graph.
    ///       It configures all GMIOs and external buffers defined in the configuration.
    aeg_err init() {
// support enable RPU cache by set aie memory attribute
#ifdef AEG_HAS_XIL_MPU
        Xil_SetMPURegion(RPU_AIE_BASE, 0x40000000U, DEVICE_NONSHARED | PRIV_RW_USER_RW);
#endif

        AEG_LOG("init----\n");
        aeg_err status = aeg_err::ok;
#ifdef __AIESIM__
        using GraphInitIpcFn = int (*)(const char *, uint32_t);
        auto init_ipc = reinterpret_cast<GraphInitIpcFn>(
            dlsym(RTLD_DEFAULT, "aeg_ipc_graph_init_ipc"));
        if (!init_ipc) {
            AEG_ERROR("%s: aeg_ipc_graph_init_ipc not found via dlsym", "init");
            return aeg_err::backend_not_ready;
        }
        AEG_LOG("AIESIM: sending GRAPH_INIT for '%s'\n", graphName.c_str());
        if (init_ipc(graphName.c_str(), static_cast<uint32_t>(graphName.size())) != 0) {
            AEG_ERROR("%s: GRAPH_INIT IPC command failed for graph '%s'",
                      "init", graphName.c_str());
            return aeg_err::aie_driver_error;
        }
#endif
        status = cfg.ConfigureExternalBuffers(xaiemem);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureExternalBuffers failed status=%d", "init", static_cast<int32_t>(status));
            return status;
        }
        status = cfg.ConfigureGmios(xaiemem);
        if (status != aeg_err::ok) {
            if ((status == aeg_err::null_argument) || (status == aeg_err::aie_driver_error) || (status == aeg_err::internal_error)) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureGmios failed status=%d", "init", static_cast<int32_t>(status));
            return status;
        }
        status = cfg.ConfigureRtps(GraphApi);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureRtps failed status=%d", "init", static_cast<int32_t>(status));
        }
        status = cfg.ConfigureSharedBuffers(GraphApi);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureSharedBuffers failed status=%d", "init", static_cast<int32_t>(status));
            return status;
        }
        return aeg_err::ok;
    }

    /// @brief Starts and runs the graph for a specified number of iterations.
    /// Executes the graph for the given number of iterations. In simulation mode,
    /// also starts PLIOs for data transfer.
    /// @param num Number of iterations to run
    /// @note In DEBUG_RUNTIME mode, additional debug code may be executed.
    aeg_err run(int32_t num) const {

#ifdef __AIESIM__
        aiesim::StartPlios();
#endif

        if (GraphApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "run");
            return aeg_err::backend_not_ready;
        }
        aeg_err run_status = err_from_abr(GraphApi->run(num));
        if (run_status != aeg_err::ok) {
            return run_status;
        }
#ifdef DEBUG_RUNTIME
        if (0) {
            XAie_LocType loc = XAie_TileLoc(12, 3);
            const uint32_t col_12_row_3 = (loc.Col << XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
            const uint32_t core_control_addr = col_12_row_3 + 0x00038000;
            const uint32_t core_status_addr = col_12_row_3 + 0x00038004;
            const uint32_t core_gate_addr = col_12_row_3 + 0x0007FF20; // aie2ps
            uint32_t d2 = 0, g0, gate = 0;
            AEG_LOG("enable core core status is %d clock is %d before gate %d\n", d2, gate, g0);
            int32_t loop = 0;
            while (loop) {
                XAie_Read32(&DevInst, core_status_addr, &d2);
                usleep(1);
            }
        }
#endif
        return aeg_err::ok;
    }

    /// @brief Transfers data from AIE to GMIO (non-blocking).
    /// Initiates a non-blocking data transfer from AIE to GMIO. The function returns
    /// immediately without waiting for completion, even if there is an error.
    /// @param gmio_name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    aeg_err aie2gm_nb(const char* gmio_name, char* data, int32_t len) {
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (gmio_api_impl == nullptr) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", "aie2gm_nb", gmio_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(gmio_api_impl->aie2gm_nb(data, len));
    }

    /// @brief Transfers data from AIE to GMIO (blocking).
    /// Performs a blocking data transfer from AIE to GMIO. The function waits
    /// for the transfer to complete before returning, and handles errors.
    /// @param gmio_name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    aeg_err aie2gm(const char* gmio_name, char* data, int32_t len) {
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (gmio_api_impl == nullptr) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", "aie2gm", gmio_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(gmio_api_impl->aie2gm(data, len));
    }

#ifdef DEBUG_RUNTIME
    /// @brief Debug function to check GMIO to AIE transfer status.
    /// Reads and prints MM2S (Memory to Stream) status register for debugging.
    /// This function is only compiled in DEBUG_RUNTIME mode.
    /// @note Used for debugging GMIO transfer operations.
    void gm2aie_check() {
        int32_t channel = 0, col = 12, row = 0;
        XAie_LocType loc = XAie_TileLoc(12, 0);
        const uint32_t col_12_row_0 = (loc.Col << XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
        uint32_t mm2sc0_status = 0x00009328U;
        uint32_t d1;
        int32_t n = 1;

        while (n-- >= 0) {
            XAie_Read32(&DevInst, col_12_row_0 + mm2sc0_status, &d1);
            AEG_LOG("mm2s status %d\n", d1);
            usleep(10);
        }
    }
#endif

    /// @brief Transfers data from GMIO to AIE (non-blocking).
    /// Initiates a non-blocking data transfer from GMIO to AIE. The function returns
    /// immediately without waiting for completion, even if there is an error.
    /// @param gmio_name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    aeg_err gm2aie_nb(const char* gmio_name, char* data, int32_t len) {
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (gmio_api_impl == nullptr) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", "gm2aie_nb", gmio_name);
            return aeg_err::backend_not_ready;
        }
        auto status = err_from_abr(gmio_api_impl->gm2aie_nb(data, len));
        if (status != aeg_err::ok) {
            return status;
        }
        return aeg_err::ok;
    }

    /// @brief Transfers data from GMIO to AIE (blocking).
    /// Performs a blocking data transfer from GMIO to AIE. The function waits
    /// for the transfer to complete before returning, and handles errors.
    /// @param gmio_name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    aeg_err gm2aie(const char* gmio_name, char* data, int32_t len) {
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name); // grab gmio api impl based on gmio name
        if (gmio_api_impl == nullptr) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", "gm2aie", gmio_name);
            return aeg_err::backend_not_ready;
        }
        auto status = err_from_abr(
            gmio_api_impl->gm2aie(data, len)); // use the gmio api impl version of gm2aie to map data from gmio to aie
        if (status != aeg_err::ok) {
            return status;
        }
        return aeg_err::ok;
    }

    // External Buffer APIs

    /// @brief Sets the address for an external buffer.
    /// Sets the address pointer for an external buffer used in data transfer operations.
    /// @param externalbuffer_name Name of the external buffer
    /// @param ptr Pointer to the address to set
    aeg_err setAddress(const char* externalbuffer_name, void* ptr){
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (externalbuffer_api_impl == nullptr) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", "setAddress", externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        externalbuffer_api_impl->setAddress(ptr);
        return aeg_err::ok;
    }

    /// @brief Waits for an external buffer operation to complete.
    /// Waits for a data transfer operation on an external buffer port to complete.
    /// @param externalbuffer_name Name of the external buffer
    /// @param port_name Name of the port to wait for
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err wait(const char* externalbuffer_name, std::string_view port_name){
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (externalbuffer_api_impl == nullptr) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", "wait", externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(externalbuffer_api_impl->wait(port_name));
    }

    /// @brief Transfers data from GMIO to AIE using external buffer (non-blocking).
    /// Initiates a non-blocking data transfer from GMIO to AIE using external buffer
    /// with tiling support. Supported on AI Engine-ML devices.
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err gm2aie_nb(const char* externalbuffer_name, std::string_view port_name){
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (externalbuffer_api_impl == nullptr) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", "gm2aie_nb", externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(externalbuffer_api_impl->gm2aie_nb(port_name));
    }

    /// @brief Transfers data from AIE to GMIO using external buffer (non-blocking).
    /// Initiates a non-blocking data transfer from AIE to GMIO using external buffer
    /// with tiling support. Supported on AI Engine-ML devices.
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err aie2gm_nb(const char* externalbuffer_name, std::string_view port_name){
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (externalbuffer_api_impl == nullptr) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", "aie2gm_nb", externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(externalbuffer_api_impl->aie2gm_nb(port_name));
    }

    /// @brief Sets up ping-pong buffers for an external buffer.
    /// Configures a ping-pong buffer system for an external buffer to enable
    /// continuous data transfer operations with alternating buffers.
    /// @param externalbuffer_name Name of the external buffer
    /// @param ptr1 Address of the first buffer pointer
    /// @param ptr2 Address of the second buffer pointer
    /// @param sync_cache If true (default), flush CPU cache before DMA. Set to
    ///        false for DMA-to-DMA buffers where the CPU never touches the data.
    /// @return aeg_err indicating success or failure
    aeg_err setupPingPongBuffers(const char* externalbuffer_name, void* const ptr1,  void* const ptr2, bool sync_cache = true){
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (externalbuffer_api_impl == nullptr) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", "setupPingPongBuffers", externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(externalbuffer_api_impl->setupPingPongBuffers(ptr1, ptr2, sync_cache));
    }

    // End external Buffer APIs

    /// @brief Updates data for an RTP or shared buffer port.
    /// Updates runtime parameters or shared buffer data for the specified port.
    /// The function automatically determines if the port is an RTP or shared buffer
    /// and calls the appropriate update function.
    /// @param port_name Name of the RTP or shared buffer port
    /// @param data Pointer to data to update
    /// @param len Number of bytes of data
    /// @return aeg_err indicating success (err_code::ok) or failure (err_code::aie_driver_error)
    aeg_err update(const std::string_view port_name, void* data, size_t len) {
        if (GraphApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "update");
            return aeg_err::backend_not_ready;
        }
        auto isRTPorSharedBuffer = cfg.isRTPorSharedBuffer(port_name);
        if (isRTPorSharedBuffer == 0) { // RTP
            auto *rt_config = cfg.get_rtp_config(port_name);
            if (rt_config == nullptr) {
                AEG_ERROR("%s: RTP config for '%.*s' not found", "update", static_cast<int32_t>(port_name.size()), port_name.data());
                return aeg_err::invalid_name;
            }
            return err_from_abr(GraphApi->update(rt_config, data, len));
        } else if (isRTPorSharedBuffer == 1) { // Shared Buffer
            auto *config = cfg.get_shared_buffer_config(port_name);
            if (config == nullptr) {
                AEG_ERROR("%s: shared buffer config for '%.*s' not found", "update", static_cast<int32_t>(port_name.size()), port_name.data());
                return aeg_err::invalid_name;
            }
            return err_from_abr(GraphApi->update(config, data, len));
        } else {
            AEG_ERROR("%s: port '%.*s' is not an RTP or shared buffer", "update", static_cast<int32_t>(port_name.size()), port_name.data());
            return aeg_err::invalid_name;
        }
    }

    /// @brief Reads data from an RTP port.
    /// Reads runtime parameter data from the specified RTP port. Performs error
    /// checking and reads data based on the RTP configuration name.
    /// @param port_name Name of the RTP port to read from
    /// @param data Pointer to buffer to store read data
    /// @param len Number of bytes to read
    /// @return aeg_err indicating success or failure
    aeg_err read(const std::string_view port_name, char *data, int32_t len) {
        if (GraphApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "read");
            return aeg_err::backend_not_ready;
        }
        auto *rt_config = cfg.get_rtp_config(port_name);
        if (rt_config == nullptr) {
            AEG_ERROR("%s: RTP config for '%.*s' not found", "read", static_cast<int32_t>(port_name.size()), port_name.data());
            return aeg_err::invalid_name;
        }
        return err_from_abr(GraphApi->read(rt_config, data, static_cast<std::size_t>(len)));
    }

    /// @brief Dumps core log for a specific tile.
    /// Retrieves and returns the core log data for the specified tile location.
    /// @param row Row index of the tile
    /// @param col Column index of the tile
    /// @return Pointer to core log string data
    char* dumpcorelog(int32_t row, int32_t col) {
        return rlog(row, col, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&DevInst)));
    }
    /// @brief Gets the device instance pointer.
    /// Returns a pointer to the underlying XAie_DevInst structure for direct
    /// access to low-level driver functions.
    /// @return Pointer to XAie_DevInst structure
    void *getaiedevinst() & { return static_cast<void *>(&DevInst); }
    const void *getaiedevinst() const & { return static_cast<const void *>(&DevInst); }
    void *getaiedevinst() && = delete;
    const void *getaiedevinst() const && = delete;

    /// @brief Gets the original AIE base address from config.
    /// Original AIE base from config (0x20000000000); unaffected by the RPU
    /// BaseAddr override, which only changed a local in the constructor.
    /// @return Original AIE base address (dconfig.base_address)
    std::uint64_t getaiebase() { return cfg.get_driver_config().base_address; }

    /// @brief Waits for graph execution to complete.
    /// Waits for all cores in the graph to finish executing their tasks.
    /// @note This function blocks until all cores have completed execution.
    aeg_err wait() const {
        if (GraphApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "wait");
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(GraphApi->wait());
    }

    /// @brief Ends graph execution and performs cleanup.
    /// Verifies that the graph is properly configured, waits for completion,
    /// checks that all cores finished running, and performs cleanup operations.
    /// @return aeg_err indicating success or failure
    aeg_err end() const {
        AEG_LOG("graph end() called\n");
        if (GraphApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "end");
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(GraphApi->end());
    }

    /// @brief Waits for a specific GMIO operation to complete.
    /// Waits for a GMIO transfer operation to complete, optionally in sync mode.
    /// Checks for any buffers in queue and syncs them if needed.
    /// @param gmio_name Name of the GMIO to wait for
    /// @param syncmode If true, waits in synchronous mode
    /// @return aeg_err indicating success or failure
    aeg_err gmio_wait(std::string_view gmio_name, bool syncmode) {
        auto gmioApiImpl = cfg.get_gmio_api_impl(gmio_name);
        if (gmioApiImpl == nullptr) {
            AEG_ERROR("%s: GMIO API impl for '%.*s' not found", "gmio_wait", static_cast<int32_t>(gmio_name.size()), gmio_name.data());
            return aeg_err::backend_not_ready;
        }
        int32_t wait_status = gmioApiImpl->wait(syncmode);
        if (wait_status != 0) {
            AEG_ERROR("%s: GMIO wait failed for '%.*s' with code=%d", "gmio_wait", static_cast<int32_t>(gmio_name.size()), gmio_name.data(), wait_status);
        }
        return static_cast<aeg_err>(wait_status);
    }

    /// @brief Allocates memory for AIE device operations.
    /// Allocates memory using the XAieMem wrapper class for AIE device operations.
    /// @param len Size of memory to allocate in bytes
    /// @return Pointer to allocated memory on success, nullptr on failure
    void* malloc(uint32_t len) const {
        if (xaiemem == nullptr) {
            AEG_ERROR("%s: XAieMem not found", "malloc");
            return nullptr;
        }
        return xaiemem->malloc(static_cast<ssize_t>(len));
    }

    /// @brief Frees memory allocated for AIE device operations.
    /// Frees memory that was previously allocated using malloc().
    /// @param addr Pointer to memory to free (must be allocated by malloc())
    aeg_err free(void* addr) const {
        if (xaiemem == nullptr) {
            AEG_ERROR("%s: XAieMem not found", "free");
            return aeg_err::backend_not_ready;
        }
        static_cast<void>(xaiemem->free(addr));
        return aeg_err::ok;
    }

    /// @brief Backtracks and collects error information.
    /// Processes error data from the device and collects error payloads.
    /// If an error handling callback is registered, it will be called with the errors.
    /// @return Pointer to collected error payloads vector
    void *BacktrackErrors() {
        backtrack_payload_cache.clear();

        if (DevInst.DevProp.DevGen == XAIE_DEV_GEN_AIE2PS) {
#ifdef __AIESIM__
            /* XAie_BacktrackErrorInterruptsCb is not exported by Vitis 2026.1
             * skip the call in sim builds.
             */
#else
            AieRC RC = XAIE_OK;

            RC = XAie_BacktrackErrorInterruptsCb(&DevInst, &error_backtrack_callback,
                                                 static_cast<void *>(this));

            if (RC != XAIE_OK) {
                AEG_ERROR("%s: XAie_BacktrackErrorInterruptCb failed rc=%d", "BacktrackErrors",
                          static_cast<int32_t>(RC));
            }
#endif /* __AIESIM__ */
        } else {
            const int32_t start_col = static_cast<int32_t>(DevInst.StartCol);
            const int32_t num_cols = static_cast<int32_t>(DevInst.NumCols);
            if ((num_cols <= 0) || (start_col >= num_cols)) {
                AEG_ERROR("%s: invalid device state (StartCol=%d, NumCols=%d)", "BacktrackErrors", start_col, num_cols);
                return nullptr;
            }

#ifdef __AIESIM__
            // XAie_BacktrackErrorInterrupts is not exported by Vitis 2026.1
            // skip the call in sim builds.
            if (error_handling_cb) {
                error_handling_cb(backtrack_payload_cache, error_handling_cb_priv);
            }
            return static_cast<void *>(backtrack_payload_cache.data());
#else
            constexpr uint32_t kPayloadSize = 32U;
            constexpr uint32_t kMaxBacktrackIterations = 16384U;
            std::array<XAie_ErrorPayload, kPayloadSize> tmp_payload{};
            XAie_ErrorMetaData MData = {};
            MData.Payload = tmp_payload.data();
            MData.ArraySize = kPayloadSize;
            MData.Cols.Start = DevInst.StartCol;
            MData.Cols.Num = DevInst.NumCols;
            uint32_t iteration = 0U;

            do {
                iteration++;
                if (iteration > kMaxBacktrackIterations) {
                    AEG_ERROR("%s: exceeded max backtrack iterations (%u)", "BacktrackErrors", kMaxBacktrackIterations);
                    return nullptr;
                }

                AieRC rc = XAie_BacktrackErrorInterrupts(&DevInst, &MData);
                if ((rc != XAIE_OK) && (rc != XAIE_INSUFFICIENT_BUFFER_SIZE)) {
                    AEG_ERROR("%s: XAie_BacktrackErrorInterrupts failed rc=%d", "BacktrackErrors", static_cast<int32_t>(rc));
                    return nullptr;
                }

                uint32_t safe_error_count = static_cast<uint32_t>(MData.ErrorCount);
                if (safe_error_count > kPayloadSize) {
                    AEG_ERROR("%s: error count %u exceeds payload buffer %u; clamping", "BacktrackErrors", safe_error_count,
                              kPayloadSize);
                    safe_error_count = kPayloadSize;
                }
                for (uint32_t i = 0; i < safe_error_count; i++) {
                    XAie_ErrorPayload *p = &tmp_payload[i];
                    backtrack_payload_cache.push_back(*p);
                }

            } while (MData.IsNextInfoValid != 0U);
#endif /* __AIESIM__ */
            if (error_handling_cb) {
                error_handling_cb(backtrack_payload_cache, error_handling_cb_priv);
            }
            if (backtrack_payload_cache.empty()) {
                return nullptr;
            }
        }
        return static_cast<void *>(backtrack_payload_cache.data());
    }

    /// @brief Starts profiling for a single port.
    /// Starts performance profiling for a specified port with the given option and value.
    /// @param port_name Name of the port to profile
    /// @param option Profiling option:
    ///               - abr::_io_total_stream_running_to_idle_cycles: Total accumulated cycles
    ///               - abr::_io_stream_start_to_bytes_transferred_cycles: Cycles from start to bytes transferred
    ///               - abr::_io_stream_start_difference_cycles: Cycles between two platform IO objects
    ///               - abr::_io_stream_running_event_count: Number of stream running events
    /// @param value Additional value for profiling configuration
    /// @return Handle to the profiling session
    PROFILE_ID start_profiling(const std::string_view port_name, int32_t option, uint32_t value) const {
        if (EventApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "start_profiling");
            return -1;
        }
        return EventApi->start_profiling(port_name, option, value);
    }

    /// @brief Starts profiling for two ports.
    /// Starts performance profiling comparing two ports with the given option and value.
    /// @param port_name1 Name of the first port to profile
    /// @param port_name2 Name of the second port to profile
    /// @param option Profiling option:
    ///               - abr::_io_total_stream_running_to_idle_cycles: Total accumulated cycles
    ///               - abr::_io_stream_start_to_bytes_transferred_cycles: Cycles from start to bytes transferred
    ///               - abr::_io_stream_start_difference_cycles: Cycles between two platform IO objects
    ///               - abr::_io_stream_running_event_count: Number of stream running events
    /// @param value Additional value for profiling configuration
    /// @return Handle to the profiling session
    PROFILE_ID start_profiling(const std::string_view port_name1,
           const std::string_view port_name2,
           int32_t option,
           uint32_t value) const {
        if (EventApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "start_profiling");
            return -1;
        }
        return EventApi->start_profiling(port_name1, port_name2, option, value);
    }

    /// @brief Reads profiling data for a profiling session.
    /// Retrieves the profiling data collected for the specified profiling handle.
    /// @param h Handle to the profiling session (returned by start_profiling)
    /// @return Profiling data value (typically cycle count or event count)
    int64_t read_profiling(PROFILE_ID h) const {
        if (EventApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "read_profiling");
            return static_cast<int64_t>(-1);
        }
        return static_cast<int64_t>(EventApi->read_profiling(h));
    }

    /// @brief Stops profiling and cleans up resources.
    /// Stops the profiling session and cleans up performance monitoring and
    /// event-related resources.
    /// @param h Handle to the profiling session to stop
    aeg_err stop_profiling(PROFILE_ID h) const {
        if (EventApi == nullptr) {
            AEG_ERROR("%s: backend not ready", "stop_profiling");
            return aeg_err::backend_not_ready;
        }
        EventApi->stop_profiling(h);
        return aeg_err::ok;
    }

    std::shared_ptr<gmioapiimpl::GmioApiImpl> getGmioApiImpl(const std::string_view name) {
        return cfg.get_gmio_api_impl(name);
    }

    std::shared_ptr<externalbufferapiimpl::ExternalBufferApiImpl> getExternalBufferApiImpl(const std::string_view name) {
        return cfg.get_externalbuffer_api_impl(name);
    }

    std::shared_ptr<rtpapiimpl::RtpApiImpl> getRtpApiImpl(const std::string_view name) { return cfg.get_rtp_api_impl(name); }

    std::shared_ptr<sharedbufferapiimpl::SharedBufferApiImpl> getSharedBufferApiImpl(const std::string_view name) {
        return cfg.get_sharedbuffer_api_impl(name);
    }

#ifdef __BAREMETAL_UTESTS__
    void reset_xaiemem() { xaiemem.reset(); }
    void reset_GraphApi() { GraphApi.reset(); }
    void reset_EventApi() { EventApi.reset(); }
#endif

  private:
    aeg_configureparser::ConfigureParser cfg{};
    XAie_DevInst DevInst = {};
    std::shared_ptr<abr::graph_api> GraphApi{nullptr};
    std::shared_ptr<abr::event> EventApi{nullptr};
    std::shared_ptr<xaiemem::XAieMem> xaiemem{nullptr};
    std::string graphName{};
    void (*error_handling_cb)(std::vector<XAie_ErrorPayload> payloads, void *priv) = nullptr;
    void *error_handling_cb_priv = nullptr;
    std::vector<XAie_ErrorPayload> backtrack_payload_cache{};
    cert_host::cert_api cert{};

    static void error_backtrack_callback(XAie_ErrorPayload *payload, void *priv) {
        AIEGraphSim *graph = static_cast<AIEGraphSim *>(priv);

        if (graph == nullptr) {
            AEG_ERROR("%s: graph pointer is null", "error_backtrack_callback");
            return;
        }
        graph->backtrack_payload_cache.clear();
        graph->backtrack_payload_cache.push_back(*payload);
        if (graph->error_handling_cb) {
            graph->error_handling_cb(graph->backtrack_payload_cache, graph->error_handling_cb_priv);
        }
        graph->backtrack_payload_cache.clear();
    }

    /// @brief Self-test function for CERT (Control Engine Runtime).
    /// Tests CERT functionality by loading CERT, waking it up, initializing handshake,
    /// and submitting a test command. Used for validation and debugging.
    /// @return 0 on success, error code on failure
    // int32_t selftest_cert() {
    //     std::int32_t ret = 0;
    //     std::map<std::string_view, uint64_t> symbols;

    //     AEG_LOG("test cert start.\n");
    //     ret = this->cert.load_cert(0);
    //     if (ret != 0) {
    //         AEG_ERROR("load cert failed: %d\n", ret);
    //     }
    //     ret = this->cert.wakeup_cert(0);
    //     if (ret != 0) {
    //         AEG_ERROR("Wakup cert col 0 failed\n");
    //     }
    //     static_cast<void>(cert.init_handshake());
    //     ret = cert.submit_test_command();

    //     if (ret != 0) {
    //         AEG_ERROR("CERT: hsa test failed: %d\n", ret);
    //     } else {
    //         AEG_LOG("CERT: hsa test PASSED.\n");
    //     }
    //     return ret;
    // }
};

struct AIEGraphImpl : public AIEGraphSim {
    using AIEGraphSim::AIEGraphSim;
    AIEGraphImpl(const AIEGraphImpl &) = delete;
    AIEGraphImpl &operator=(const AIEGraphImpl &) = delete;
    AIEGraphImpl(AIEGraphImpl &&) = delete;
    AIEGraphImpl &operator=(AIEGraphImpl &&) = delete;
    ~AIEGraphImpl() override = default;
};

namespace {
// Graphs are often globals in other translation units, so AIEGraph's
// constructor can run before this file's dynamic initializers. A null
// unique_ptr is constant-initialized, so it is valid before any of them run.
std::unique_ptr<std::vector<std::shared_ptr<AIEGraphSim>>> graph_sim_list;
} // namespace

/// @brief Allocates memory for AIE device operations.
/// Wrapper function that allocates memory using the underlying graph's memory allocator.
/// @param len Size of memory to allocate in bytes
/// @return Pointer to allocated memory on success, nullptr on failure
/// @see AIEGraphSim::malloc()
void *AIEGraph::malloc(size_t len) const {
    if (len == 0) {
        AEG_ERROR("%s: invalid length argument\n", "malloc");
        return nullptr;
    }

    return graph->malloc(static_cast<std::uint32_t>(len));
}

/// @brief Frees memory allocated for AIE device operations.
/// Wrapper function that frees memory using the underlying graph's memory allocator.
/// @param mem Pointer to memory to free (must be allocated by malloc())
/// @see AIEGraphSim::free()
aeg_err AIEGraph::free(void *mem) const {
    return graph->free(mem);
}
/**
 * @brief  Creates a new Baremetal Graph object.
 * @param  GraphName       Name of graph.
 */
AIEGraph::AIEGraph(std::string_view GraphName) : isInitialized(false) {
    graph = std::make_shared<AIEGraphImpl>(GraphName);

    if (graph_sim_list == nullptr) {
        graph_sim_list = std::make_unique<std::vector<std::shared_ptr<AIEGraphSim>>>();
        graph_sim_list->push_back(graph);
        abr_init_isr();
    } else {
        graph_sim_list->push_back(graph);
    }
}

/// @brief Destructor for AIEGraph.
/// Cleans up the graph instance when the object is destroyed.
AIEGraph::~AIEGraph() = default;

/// @brief Verifies AIEGraph has been initialized.
/// Checks internal initialized state and emits a user-facing error message when API
/// calls are made before init().
/// @param function_name Caller name used in the emitted error message
/// @return true when initialized, false otherwise
bool AIEGraph::check_initialized_impl() const {
    if (!isInitialized) {
        AEG_ERROR("AIEGraph: invalid state (graph not initialized; call init() first)");
        return false;
    }
    return true;
}

aeg::detail::GraphImplPtr AIEGraph::getGraphImpl() const {
    return static_cast<aeg::detail::GraphImplPtr>(static_cast<void *>(graph.get()));
}

/// @brief Performs self-test and exits.
/// Runs self-test functions including error handling tests and clock enable/disable tests.
/// This is typically used for debugging and validation purposes.
aeg_err AIEGraph::selftestexit() const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "selftestexit");
        return aeg_err::invalid_state;
    }
    return graph->selftestexit();
}

/// @brief Enables clock for AIE columns.
/// Enables the clock for all columns in the device partition.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::clock_enable()
aeg_err AIEGraph::clock_enable() const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "clock_enable");
        return aeg_err::invalid_state;
    }
    return graph->clock_enable();
}

/// @brief Disables clock for AIE columns.
/// Disables the clock for all columns in the device partition.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::clock_disable()
aeg_err AIEGraph::clock_disable() const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "clock_disable");
        return aeg_err::invalid_state;
    }
    return graph->clock_disable();
}

/// @brief Sets up callback function for error handling.
/// Registers a callback function that will be called when errors are detected.
/// @param cb Pointer to the callback function
/// @param priv Private data pointer passed to the callback function
/// @see AIEGraphSim::error_handling_cb_init()
aeg_err AIEGraph::error_handling_cb_init(void *cb, void *priv) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "error_handling_cb_init");
        return aeg_err::invalid_state;
    }
    if (cb == nullptr) {
        AEG_ERROR("%s: null argument", "error_handling_cb_init");
        return aeg_err::null_argument;
    }
    return graph->error_handling_cb_init(cb, priv);
}

/// @brief Initializes the graph and configures GMIOs and external buffers.
/// Optionally applies a set of CDO blobs first, then sets up GMIOs and external
/// buffers for data transfer. This must be called before running the graph.
///
/// The CDOs are deliberately applied here rather than being left to the caller.
/// A CDO set containing aie_cdo_reset.bin column-resets every column and
/// shim-resets every shim, which clears the shim DMA channel-enable bits that
/// ConfigureGmios() -> gmio_api::configure() programs below, and that no CDO
/// restores. Loading CDOs after the GMIO setup therefore leaves every shim DMA
/// channel disabled and hangs the first gmio wait in XAie_DmaWaitForDone.
/// Doing both here makes that ordering impossible for a caller to get wrong.
/// @param blobs Array of CDO blobs to apply before init, or nullptr for none.
/// @param count Number of entries in @p blobs.
/// @see AIEGraphSim::init()
aeg_err AIEGraph::init(const aeg::cdo_blob *blobs, std::size_t count) {
    if (graph == nullptr) {
        AEG_ERROR("%s: backend not ready", "init");
        return aeg_err::backend_not_ready;
    }
    if ((blobs == nullptr) && (count != 0U)) {
        AEG_ERROR("%s: null blob array with count=%" PRIu64, "init",
                  static_cast<std::uint64_t>(count));
        return aeg_err::null_argument;
    }

    // Apply the CDOs before any runtime configuration touches the array.
    for (std::size_t i = 0U; i < count; ++i) {
        if (blobs[i].data == nullptr) {
            AEG_ERROR("%s: CDO blob %" PRIu64 " has a null data pointer", "init",
                      static_cast<std::uint64_t>(i));
            return aeg_err::null_argument;
        }
        if (blobs[i].len == 0U) {
            AEG_ERROR("%s: CDO blob %" PRIu64 " has zero length", "init",
                      static_cast<std::uint64_t>(i));
            return aeg_err::invalid_length;
        }
        const aeg_err cdo_status =
            load_cdo(reinterpret_cast<uintptr_t>(blobs[i].data), blobs[i].len);
        if (cdo_status != aeg_err::ok) {
            AEG_ERROR("%s: load_cdo failed for CDO blob %" PRIu64 " status=%d",
                      "init", static_cast<std::uint64_t>(i),
                      static_cast<std::int32_t>(cdo_status));
            return cdo_status;
        }
    }

    aeg_err status = graph->init();
    if (status != aeg_err::ok) {
        return status;
    }
    isInitialized = true;
    return aeg_err::ok;
}
#ifdef __BAREMETAL_UTESTS__
 void AIEGraph::reset_xaiemem() { graph->reset_xaiemem(); }
 void AIEGraph::reset_GraphApi() { graph->reset_GraphApi(); }
 void AIEGraph::reset_EventApi() { graph->reset_EventApi(); }
#endif
/// @brief Starts and runs the graph for a specified number of iterations.
/// Executes the graph for the given number of iterations. In simulation mode,
/// also starts PLIOs for data transfer.
/// @param num Number of iterations to run
/// @see AIEGraphSim::run()
aeg_err AIEGraph::run(std::int32_t num) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "run");
        return aeg_err::invalid_state;
    }
    if ((num == 0) || (num < -1)) {
        AEG_ERROR("%s: invalid length argument", "run");
        return aeg_err::invalid_length;
    }
    return graph->run(num);
}

/// @brief Transfers data from AIE to GMIO (non-blocking).
/// Initiates a non-blocking data transfer from AIE to GMIO. The function returns
/// immediately without waiting for completion.
/// @param name Name of the GMIO to use for transfer
/// @param data Pointer to data buffer to transfer
/// @param len Length of data in bytes
/// @see AIEGraphSim::aie2gm_nb()
aeg_err AIEGraph::aie2gm_nb(const char *name, char *data, int32_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "aie2gm_nb");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm_nb");
        return aeg_err::invalid_length;
    }
    return graph->aie2gm_nb(name, data, len);
}

/// @brief Transfers data from GMIO to AIE (non-blocking).
/// Initiates a non-blocking data transfer from GMIO to AIE. The function returns
/// immediately without waiting for completion.
/// @param name Name of the GMIO to use for transfer
/// @param data Pointer to data buffer to transfer
/// @param len Length of data in bytes
/// @see AIEGraphSim::gm2aie_nb()
aeg_err AIEGraph::gm2aie_nb(const char *name, char *data, int32_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "gm2aie_nb");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie_nb");
        return aeg_err::invalid_length;
    }
    return graph->gm2aie_nb(name, data, len);
}
/// @brief Transfers data from AIE to GMIO (blocking).
/// Performs a blocking data transfer from AIE to GMIO. The function waits
/// for the transfer to complete before returning.
/// @param name Name of the GMIO to use for transfer
/// @param data Pointer to data buffer to transfer
/// @param len Length of data in bytes
/// @see AIEGraphSim::aie2gm()
aeg_err AIEGraph::aie2gm(const char* name, char* data, int32_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "aie2gm");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", "aie2gm");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm");
        return aeg_err::invalid_length;
    }
    return graph->aie2gm(name, data, len);
}

/// @brief Transfers data from GMIO to AIE (blocking).
/// Performs a blocking data transfer from GMIO to AIE. The function waits
/// for the transfer to complete before returning.
/// @param name Name of the GMIO to use for transfer
/// @param data Pointer to data buffer to transfer
/// @param len Length of data in bytes
/// @see AIEGraphSim::gm2aie()
aeg_err AIEGraph::gm2aie(const char* name, char* data, int32_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "gm2aie");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", "gm2aie");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie");
        return aeg_err::invalid_length;
    }
    return graph->gm2aie(name, data, len);
}

/// @brief Waits for graph execution to complete.
/// Waits for all cores in the graph to finish executing their tasks.
/// @see AIEGraphSim::wait()
aeg_err AIEGraph::wait() const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "wait");
        return aeg_err::invalid_state;
    }
    return graph->wait();
}

/// @brief Ends graph execution and performs cleanup.
/// Verifies that the graph is properly configured, waits for completion,
/// checks that all cores finished running, and performs cleanup operations.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::end()
aeg_err AIEGraph::end() const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "end");
        return aeg_err::invalid_state;
    }
    return graph->end();
}

/// @brief Waits for a specific GMIO operation to complete.
/// Waits for a GMIO transfer operation to complete, optionally in sync mode.
/// @param gmio_name Name of the GMIO to wait for
/// @param syncmode If true, waits in synchronous mode
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::gmio_wait()
aeg_err AIEGraph::gmio_wait(std::string_view gmio_name,bool syncmode) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "gmio_wait");
        return aeg_err::invalid_state;
    }
    if (gmio_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "gmio_wait");
        return aeg_err::invalid_name;
    }
    return graph->gmio_wait(gmio_name, syncmode);
}

/// @brief Updates data for an RTP or shared buffer port.
/// Updates runtime parameters or shared buffer data for the specified port.
/// The function automatically determines if the port is an RTP or shared buffer.
/// @param port_name Name of the RTP or shared buffer port
/// @param data Pointer to data to update
/// @param len Number of bytes of data
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::update()
aeg_err AIEGraph::update(std::string_view port_name, void *data, size_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "update");
        return aeg_err::invalid_state;
    }
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "update");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "update");
        return aeg_err::null_argument;
    }
    if (len == 0) {
        AEG_ERROR("%s: invalid length argument", "update");
        return aeg_err::invalid_length;
    }
    return graph->update(port_name, data, len);
}

/// @brief Dumps core log for a specific tile.
/// Retrieves and returns the core log data for the specified tile location.
/// @param row Row index of the tile
/// @param col Column index of the tile
/// @return Pointer to core log string data
/// @see AIEGraphSim::dumpcorelog()
char* AIEGraph::dumpcorelog(int32_t row, int32_t col) const {
    if (!check_initialized_impl()) {
        return nullptr;
    }
    if ((row < 0) || (col < 0)) {
        return nullptr;
    }
    return graph->dumpcorelog(row, col);
}

/// @brief Gets the device instance pointer.
/// Returns a pointer to the underlying XAie_DevInst structure for direct
/// access to low-level driver functions.
/// @return Pointer to XAie_DevInst structure
/// @see AIEGraphSim::getaiedevinst()
void* AIEGraph::getaiedevinst() const {
    if (!check_initialized_impl()) {
        return nullptr;
    }
    return graph->getaiedevinst();
}

// ============================================================================
// CDO (Configuration Data Object) loader.
//
// Integrated from the former aeg_cdo.cc and aeg_cdo_parser.cc translation
// units. Provides the module-level aeg::cdo::parse_and_apply()/load() and the
// AIEGraph::load_cdo() member (declared in aeg_baremetal_api.h / aeg_cdo.h).
//
// The command-decoding logic mirrors the device loader XCdo_LoadCdo
// (thirdparty/aie-pdi-transform/lib/cdo_cmd.c) and the command-size helper
// XCdo_CmdSize (cdo_cmd.h). Unlike that reference -- which has a missing
// break after the WRITE64 case that lets it fall through into NOP -- every
// case here terminates with break.
// ============================================================================
namespace aeg {
namespace cdo {

namespace {

// --- CDO format constants (inlined from cdo_cmd.h) ------------------------

/// Number of 32-bit words in the CDO header.
constexpr std::uint32_t kCdoHdrLen = 0x5U;
/// Header identification word ("CDO\0").
constexpr std::uint32_t kCdoHdrIdnWrd = 0x004F4443U;

/// Mask selecting the command API id from a command's first word.
constexpr std::uint32_t kCmdApiIdMask = 0xFFU;
/// Mask/shift selecting the short-command payload length from the first word.
constexpr std::uint32_t kCmdLenMask = 0xFF0000U;
constexpr std::uint32_t kCmdLenShift = 16U;
/// A short length field equal to this signals a long command (length in word1).
constexpr std::uint32_t kMaxShortCmdLen = 255U;
/// Header word count for short and long commands respectively.
constexpr std::uint32_t kShortCmdHdrLen = 1U;
constexpr std::uint32_t kLongCmdHdrLen = 2U;

// CDO command opcodes.
constexpr std::uint32_t kCmdMaskWrite = 2U;
constexpr std::uint32_t kCmdWrite = 3U;
constexpr std::uint32_t kCmdDmaWrite = 5U;
constexpr std::uint32_t kCmdMaskPoll64 = 6U;
constexpr std::uint32_t kCmdMaskWrite64 = 7U;
constexpr std::uint32_t kCmdWrite64 = 8U;
constexpr std::uint32_t kCmdNop = 17U;

/// Decoded command: pointer to the payload words and the counts we need.
struct DecodedCmd {
    const std::uint32_t *payload; ///< First payload word (after the header).
    std::uint32_t payload_len;    ///< Payload length in words.
    std::uint32_t size;           ///< Total command size in words (hdr+payload).
};

/// Decode command size/payload from the first word(s), mirroring XCdo_CmdSize.
DecodedCmd decode_cmd(const std::uint32_t *buf) {
    DecodedCmd cmd;
    const std::uint32_t word0 = buf[0];
    std::uint32_t size = kShortCmdHdrLen;
    std::uint32_t payload_len = (word0 & kCmdLenMask) >> kCmdLenShift;

    if (payload_len == kMaxShortCmdLen) {
        size = kLongCmdHdrLen;
        payload_len = buf[1];
        cmd.payload = &buf[kLongCmdHdrLen];
    } else {
        cmd.payload = &buf[kShortCmdHdrLen];
    }
    cmd.size = size + payload_len;
    cmd.payload_len = payload_len;
    return cmd;
}

/// Translate a CDO absolute AIE address into the register offset the XAie
/// driver expects (relative to the device instance base address).
inline std::uint64_t cdo_reg_off(std::uint64_t cdo_addr,
                             const XAie_DevInst *dev,
                             std::uint64_t cdo_base) {
    (void)dev; // base now supplied explicitly via cdo_base
    auto cdo_reg_off = cdo_addr;
    if ((cdo_reg_off & cdo_base) != 0U) {
        cdo_reg_off = (cdo_addr & ~cdo_base) | dev->BaseAddr;
    }
    AEG_CDO_LOG("cdo_addr cdo_reg_off: cdo_addr is 0x%llx cdo_reg_off is 0x%llx\n",
            static_cast<uint64_t>(cdo_addr),
            static_cast<uint64_t>(cdo_reg_off));
    return cdo_reg_off;
}

} // namespace

aeg_err parse_and_apply(const void *data, std::size_t len, XAie_DevInst *dev,
                        std::uint64_t cdo_base) {
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    AEG_LOG("cdo load not support in SIMU yest\n");
    return aeg_err::ok;
#endif
    if (data == nullptr) {
        AEG_ERROR("cdo: null blob pointer");
        return aeg_err::user_error;
    }
    if (dev == nullptr) {
        AEG_ERROR("cdo: null device instance");
        return aeg_err::user_error;
    }

    // Need at least the full header plus one command word.
    const std::size_t total_words = len / sizeof(std::uint32_t);
    if (total_words <= kCdoHdrLen) {
        AEG_ERROR("cdo: blob too short: %" PRIu64 " bytes",
                  static_cast<std::uint64_t>(len));
        return aeg_err::user_error;
    }

    AEG_LOG("aeglog dev->BaseAddr is %" PRIx64 " &dev->BaseAddr is %p\n",
            static_cast<std::uint64_t>(dev->BaseAddr),
            static_cast<void *>(&dev->BaseAddr));
    const std::uint32_t *hdr = static_cast<const std::uint32_t *>(data);

    // Verify identification magic (word[1]).
    if (hdr[1] != kCdoHdrIdnWrd) {
        AEG_ERROR("cdo: header identification failed: 0x%08x",
                  static_cast<std::uint32_t>(hdr[1]));
        return aeg_err::user_error;
    }

    // Verify checksum: word[4] == ~(sum of words[0..3]).
    std::uint32_t checksum = 0U;
    for (std::uint32_t i = 0U; i < (kCdoHdrLen - 1U); ++i) {
        checksum += hdr[i];
    }
    checksum ^= 0xFFFFFFFFU;
    if (checksum != hdr[kCdoHdrLen - 1U]) {
        AEG_ERROR("cdo: checksum failed: computed 0x%08x, expected 0x%08x",
                  static_cast<std::uint32_t>(checksum),
                  static_cast<std::uint32_t>(hdr[kCdoHdrLen - 1U]));
        return aeg_err::user_error;
    }

    // Payload length in words (word[3]) must fit within the blob.
    const std::uint32_t buf_len = hdr[3];
    if (buf_len > (total_words - kCdoHdrLen)) {
        AEG_ERROR("cdo: invalid payload length: BufLen=%u, available=%" PRIu64,
                  static_cast<std::uint32_t>(buf_len),
                  static_cast<std::uint64_t>(total_words - kCdoHdrLen));
        return aeg_err::user_error;
    }

    AEG_LOG("cdo: parsing blob: %" PRIu64 " bytes, BufLen=%u words\n",
            static_cast<std::uint64_t>(len),
            static_cast<std::uint32_t>(buf_len));

    const std::uint32_t *buf = &hdr[kCdoHdrLen];
    std::uint32_t remaining = buf_len;

    while (remaining != 0U) {
        // Need at least one word to read the command header.
        DecodedCmd cmd = decode_cmd(buf);
        const std::uint32_t cmd_id = buf[0] & kCmdApiIdMask;

        if ((cmd.size == 0U) || (cmd.size > remaining)) {
            AEG_ERROR("cdo: invalid command size %u (remaining %u), id %u",
                      static_cast<std::uint32_t>(cmd.size),
                      static_cast<std::uint32_t>(remaining),
                      static_cast<std::uint32_t>(cmd_id));
            return aeg_err::user_error;
        }

        AieRC rc = XAIE_OK;
        switch (cmd_id) {
        case kCmdWrite: {
            // Payload: [addr, value].
            const std::uint64_t addr = static_cast<std::uint64_t>(cmd.payload[0]);
            const std::uint32_t value = cmd.payload[1];
            AEG_CDO_LOG("write 0x%llx 0x%x\n",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(value));
            rc = cdo_reg_write32( cdo_reg_off(addr, dev, cdo_base), value);
            break;
        }
        case kCmdMaskWrite: {
            // Payload: [addr, mask, value].
            const std::uint64_t addr = static_cast<std::uint64_t>(cmd.payload[0]);
            const std::uint32_t mask = cmd.payload[1];
            const std::uint32_t value = cmd.payload[2];
            AEG_CDO_LOG("mask_write 0x%llx 0x%08x 0x%x",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(mask),
                    static_cast<uint32_t>(value));
            rc = cdo_reg_maskwrite32( cdo_reg_off(addr, dev, cdo_base), mask, value);
            break;
        }
        case kCmdWrite64: {
            // Payload: [addr_hi, addr_lo, value].
            const std::uint64_t addr =
                (static_cast<std::uint64_t>(cmd.payload[0]) << 32U) |
                cmd.payload[1];
            const std::uint32_t value = cmd.payload[2];
            AEG_CDO_LOG("write 0x%llx 0x%x",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(value));
            rc = cdo_reg_write32( cdo_reg_off(addr, dev, cdo_base), value);
            break;
        }
        case kCmdMaskWrite64: {
            // Payload: [addr_hi, addr_lo, mask, value].
            const std::uint64_t addr =
                (static_cast<std::uint64_t>(cmd.payload[0]) << 32U) |
                cmd.payload[1];
            const std::uint32_t mask = cmd.payload[2];
            const std::uint32_t value = cmd.payload[3];
            AEG_CDO_LOG("mask_write 0x%llx 0x%x %x",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(mask),
                    static_cast<uint32_t>(value));
            rc = cdo_reg_maskwrite32( cdo_reg_off(addr, dev, cdo_base), mask, value);
            break;
        }
        case kCmdMaskPoll64: {
            // Payload: [addr_hi, addr_lo, mask, value, timeout_us].
            if (cmd.payload_len < 5U) {
                AEG_ERROR("cdo: MASK_POLL64 payload too short: %u words",
                          static_cast<std::uint32_t>(cmd.payload_len));
                return aeg_err::user_error;
            }
            const std::uint64_t addr =
                (static_cast<std::uint64_t>(cmd.payload[0]) << 32U) |
                cmd.payload[1];
            const std::uint32_t mask = cmd.payload[2];
            const std::uint32_t value = cmd.payload[3];
            const std::uint32_t timeout_us = cmd.payload[4];
            AEG_CDO_LOG("mask_poll 0x%llx %x %x 0x%x",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(mask),
                    static_cast<uint32_t>(value),
                    static_cast<uint32_t>(timeout_us));
            rc = cdo_reg_maskpoll( cdo_reg_off(addr, dev, cdo_base), mask, value,
                              timeout_us);
            break;
        }
        case kCmdDmaWrite: {
            // Payload: [addr_hi, addr_lo, data0, data1, ...].
            // The inline data occupies (payload_len - 2) words.
            if (cmd.payload_len < 2U) {
                AEG_ERROR("cdo: DMAWRITE payload too short: %u words",
                          static_cast<std::uint32_t>(cmd.payload_len));
                return aeg_err::user_error;
            }
            const std::uint64_t addr =
                (static_cast<std::uint64_t>(cmd.payload[0]) << 32U) |
                cmd.payload[1];
            const std::uint32_t *src = &cmd.payload[2];
            const std::uint32_t count = cmd.payload_len - 2U;
            AEG_CDO_LOG("cdo: DMAWRITE addr=0x%llx words=%u",
                    static_cast<uint64_t>(addr),
                    static_cast<uint32_t>(count));
            rc = cdo_reg_blockwrite32( cdo_reg_off(addr, dev, cdo_base), src, count);
            break;
        }
        case kCmdNop:
            // Padding command: nothing to apply.
            break;
        default:
            AEG_ERROR("cdo: unsupported command id %u",
                      static_cast<std::uint32_t>(cmd_id));
            return aeg_err::user_error;
        }

        if (rc != XAIE_OK) {
            AEG_ERROR("cdo: XAie driver write failed (rc=%d) for command id %u",
                      static_cast<std::int32_t>(rc), static_cast<std::uint32_t>(cmd_id));
            return aeg_err::aie_driver_error;
        }

        buf += cmd.size;
        remaining -= cmd.size;
    }

    AEG_LOG("cdo: parse complete\n");
    return aeg_err::ok;
}

aeg_err load(const void *data, std::size_t len, XAie_DevInst *dev,
             std::uint64_t cdo_base) {
    // Forward to the self-contained parser, which validates the blob header
    // and applies each command through the XAie driver.
    return parse_and_apply(data, len, dev, cdo_base);
}

} // namespace cdo
} // namespace aeg

/// @brief Load an in-memory CDO blob into the device.
/// Prints the function name together with the address and length arguments,
/// resolves the device instance, then forwards to aeg::cdo::load().
///
/// @note This deliberately does NOT go through check_initialized_impl(). A CDO
/// set that contains aie_cdo_reset.bin performs a column reset and an NPI shim
/// reset of the whole array, which clears the shim DMA channel-enable bits that
/// init() -> ConfigureGmios() -> gmio_api::configure() programs (and that no CDO
/// restores). Loading a CDO after init() therefore leaves every shim DMA channel
/// disabled and hangs the first gmio wait in XAie_DmaWaitForDone. CDOs must be
/// applied before init(), so the device instance is resolved straight from the
/// backend, which is valid as soon as the AIEGraphSim constructor has run.
/// @param addr Base memory address of the CDO blob.
/// @param len  Length of the CDO blob in bytes.
aeg_err AIEGraph::load_cdo(uintptr_t addr, size_t len) const {
    if(addr == 0) {
        AEG_ERROR("%s: invalid address 0x%lx", "load_cdo", addr);
        return aeg_err::backend_not_ready;
    }
    if(len == 0) {
        AEG_ERROR("%s: invalid length %lu", "load_cdo", len);
        return aeg_err::invalid_length;
    }
    AEG_LOG("%s: addr=0x%" PRIx64 " len=%" PRIu64 "\n", "load_cdo",
            static_cast<std::uint64_t>(addr),
            static_cast<std::uint64_t>(len));
    if (graph == nullptr) {
        AEG_ERROR("%s: backend not ready", "load_cdo");
        return aeg_err::backend_not_ready;
    }
    auto *dev = static_cast<XAie_DevInst *>(graph->getaiedevinst());
    if (dev == nullptr) {
        AEG_ERROR("%s: device instance not available", "load_cdo");
        return aeg_err::backend_not_ready;
    }
    // Because this runs before init(), the isInitialized gate cannot be used.
    // Check the driver's own readiness flag instead: the AIEGraphSim constructor
    // bails out early on several failure paths (missing graph config, failed
    // XAie_CfgInitialize) and leaves DevInst zeroed, which would otherwise send
    // every CDO register write to base address 0.
    if (dev->IsReady != XAIE_COMPONENT_IS_READY) {
        AEG_ERROR("%s: device instance not initialized (IsReady=%u)", "load_cdo",
                  static_cast<std::uint32_t>(dev->IsReady));
        return aeg_err::backend_not_ready;
    }
    const std::uint64_t cdo_base = graph->getaiebase();        // original 0x20000000000
#if defined(ARMR5) || (defined(__ZEPHYR__) && !defined(__aarch64__))
    // RPU: keep forced alias base; strip the original CDO base in cdo_reg_off.
#else
    // APU: unchanged — absolute addressing handles both NPI and AIE.
#endif
    return aeg::cdo::load(reinterpret_cast<const void *>(addr), len,
                          dev, cdo_base);
}

/// @brief Backtracks and collects error information.
/// Processes error data from the device and collects error payloads.
/// If an error handling callback is registered, it will be called with the errors.
/// @return Pointer to collected error payloads
/// @see AIEGraphSim::BacktrackErrors()
void* AIEGraph::BacktrackErrors() const {
    if (!check_initialized_impl()) {
        return nullptr;
    }
    return graph->BacktrackErrors();
}

/// @brief Reads data from an RTP port.
/// Reads runtime parameter data from the specified RTP port.
/// @param port_name Name of the RTP port to read from
/// @param data Pointer to buffer to store read data
/// @param len Number of bytes to read
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::read()
aeg_err AIEGraph::read(std::string_view port_name, char *data, int32_t len) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "read");
        return aeg_err::invalid_state;
    }
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "read");
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "read");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "read");
        return aeg_err::invalid_length;
    }
    return graph->read(port_name, data, len);
}

// External Buffer APIs
/// @brief Sets the address for an external buffer.
/// Sets the address pointer for an external buffer used in data transfer operations.
/// @param externalbuffer_name Name of the external buffer
/// @param ptr Pointer to the address to set
/// @see AIEGraphSim::setAddress()
aeg_err AIEGraph::setAddress(const char* externalbuffer_name, void* ptr) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "setAddress");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name)) {
        AEG_ERROR("%s: invalid name argument", "setAddress");
        return aeg_err::invalid_name;
    }
    if (ptr == nullptr) {
        AEG_ERROR("%s: null argument", "setAddress");
        return aeg_err::null_argument;
    }
    return graph->setAddress(externalbuffer_name, ptr);
}

/// @brief Waits for an external buffer operation to complete.
/// Waits for a data transfer operation on an external buffer port to complete.
/// @param externalbuffer_name Name of the external buffer
/// @param port_name Name of the port to wait for
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::wait()
aeg_err AIEGraph::wait(const char* externalbuffer_name, std::string_view port_name) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "wait");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "wait");
        return aeg_err::invalid_name;
    }
    return graph->wait(externalbuffer_name, port_name);
}

/// @brief Transfers data from GMIO to AIE using external buffer (non-blocking).
/// Initiates a non-blocking data transfer from GMIO to AIE using external buffer
/// with tiling support. Supported on AIE-ML devices.
/// @param externalbuffer_name Name of the external buffer to use
/// @param port_name Name of the port for the transfer
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::gm2aie_nb()
aeg_err AIEGraph::gm2aie_nb(const char* externalbuffer_name, std::string_view port_name) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "gm2aie_nb");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    return graph->gm2aie_nb(externalbuffer_name, port_name);
}

/// @brief Transfers data from AIE to GMIO using external buffer (non-blocking).
/// Initiates a non-blocking data transfer from AIE to GMIO using external buffer
/// with tiling support. Supported on AIE-ML devices.
/// @param externalbuffer_name Name of the external buffer to use
/// @param port_name Name of the port for the transfer
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::aie2gm_nb()
aeg_err AIEGraph::aie2gm_nb(const char* externalbuffer_name, std::string_view port_name) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "aie2gm_nb");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    return graph->aie2gm_nb(externalbuffer_name, port_name);
}

/// @brief Sets up ping-pong buffers for an external buffer.
/// Configures a ping-pong buffer system for an external buffer to enable
/// continuous data transfer operations.
/// @param externalbuffer_name Name of the external buffer
/// @param ptr1 Address of the first buffer pointer
/// @param ptr2 Address of the second buffer pointer
/// @param sync_cache If true (default), flush CPU cache before DMA. Set to
///        false for DMA-to-DMA buffers where the CPU never touches the data.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::setupPingPongBuffers()
aeg_err AIEGraph::setupPingPongBuffers(const char* externalbuffer_name, void* const ptr1, void* const ptr2, bool sync_cache) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "setupPingPongBuffers");
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name)) {
        AEG_ERROR("%s: invalid name argument", "setupPingPongBuffers");
        return aeg_err::invalid_name;
    }
    if ((ptr1 == nullptr) || (ptr2 == nullptr)) {
        AEG_ERROR("%s: nullptr argument", "setupPingPongBuffers");
        return aeg_err::null_argument;
    }
    return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2, sync_cache);
}
// End external Buffer API

/// @brief Starts profiling for a single port.
/// Starts performance profiling for a specified port with the given option and value.
/// @param port_name Name of the port to profile
/// @param option Profiling option (e.g., stream running cycles, event count)
/// @param value Additional value for profiling configuration
/// @return Handle to the profiling session
/// @see AIEGraphSim::start_profiling()
PROFILE_ID AIEGraph::start_profiling(std::string_view port_name, int32_t option, uint32_t value) const {
    if (!check_initialized_impl()) {
        return -1;
    }
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid profiling port name", "start_profiling");
        return -1;
    }
    if (!is_valid_profiling_option(option)) {
        AEG_ERROR("%s: invalid profiling option=%d", "start_profiling", option);
        return -1;
    }

    return graph->start_profiling(port_name, option, value);
}

/// @brief Starts profiling for two ports.
/// Starts performance profiling comparing two ports with the given option and value.
/// @param port_name1 Name of the first port to profile
/// @param port_name2 Name of the second port to profile
/// @param option Profiling option (e.g., stream start difference cycles)
/// @param value Additional value for profiling configuration
/// @return Handle to the profiling session
/// @see AIEGraphSim::start_profiling()
PROFILE_ID AIEGraph::start_profiling(std::string_view port_name1, std::string_view port_name2, int32_t option, uint32_t value) const {
    if (!check_initialized_impl()) {
        return -1;
    }
    if (port_name1.empty() || port_name2.empty()) {
        AEG_ERROR("%s: invalid profiling port name(s)", "start_profiling");
        return -1;
    }
    if (!is_valid_profiling_option(option)) {
        AEG_ERROR("%s: invalid profiling option=%d", "start_profiling", option);
        return -1;
    }

    return graph->start_profiling(port_name1, port_name2, option, value);
}

/// @brief Reads profiling data for a profiling session.
/// Retrieves the profiling data collected for the specified profiling handle.
/// @param h Handle to the profiling session (returned by start_profiling)
/// @return Profiling data value (typically cycle count or event count)
/// @see AIEGraphSim::read_profiling()
int64_t AIEGraph::read_profiling(PROFILE_ID h) const {
    if (!check_initialized_impl()) {
        return static_cast<std::int64_t>(-1);
    }
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "read_profiling", h);
        return static_cast<std::int64_t>(-1);
    }

    return graph->read_profiling(h);
}

/// @brief Stops profiling and cleans up resources.
/// Stops the profiling session and cleans up performance monitoring resources.
/// @param h Handle to the profiling session to stop
/// @see AIEGraphSim::stop_profiling()
aeg_err AIEGraph::stop_profiling(PROFILE_ID h) const {
    if (!check_initialized_impl()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", "stop_profiling");
        return aeg_err::invalid_state;
    }
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "stop_profiling", h);
        return aeg_err::invalid_state;
    }
    return graph->stop_profiling(h);
}

namespace {
int32_t get_aie_irq_id() {
    int32_t IrqId = 180;
    uint32_t aie_gen = 0U;

    if (graph_sim_list != nullptr) {
        for (const std::shared_ptr<AIEGraphSim> &graph : *graph_sim_list) {
            XAie_DevInst *dev = static_cast<XAie_DevInst *>(graph->getaiedevinst());
            if (dev == nullptr) {
                continue;
            }
            aie_gen = static_cast<uint32_t>(dev->DevProp.DevGen);
            switch (aie_gen) {
                case XAIE_DEV_GEN_AIE2PS:
                    IrqId = static_cast<int32_t>(0xce + 32U);
                    return IrqId;
                case XAIE_DEV_GEN_AIEML:
                default:
                    IrqId = 180;
                    return IrqId;
            }
        }
    }

    return IrqId;
}
} // namespace

/// @brief Error interrupt service routine.
/// Handles AIE error interrupts by disabling interrupts, backtracking errors
/// for all graphs, and re-enabling interrupts. This is called automatically
/// by the interrupt controller when an AIE error occurs.
/// @param data Unused parameter (required by interrupt handler signature)
/// @note This function is only active on hardware (not in simulation).
void abr_error_isr(void *data) {
    int32_t IrqId = get_aie_irq_id();
    static_cast<void>(data);
    static_cast<void>(IrqId);

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
    XScuGic_Disable(&xInterruptController, static_cast<u32>(IrqId));

    if (graph_sim_list != nullptr) {
        for (const std::shared_ptr<AIEGraphSim> &graph : *graph_sim_list) {
            static_cast<void>(graph->BacktrackErrors());
        }
    }
    XScuGic_Enable(&xInterruptController, static_cast<u32>(IrqId));
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__) || defined(__AIELINUX__)) */
}

#ifdef __AIESIM__
/*
     for simulator dll export
*/
/// @brief Implementation class for BaremetalGraphSimBase interface.
/// This class provides the implementation for the simulator DLL export interface.
/// It wraps the AIEGraph class and forwards all calls to it.
/// @note This class is only compiled in AIE simulation mode.
class BaremetalGraphSimBaseImpl : public BaremetalGraphSimBase {
  public:
    /// @brief Constructor for BaremetalGraphSimBaseImpl.
    /// Creates an AIEGraph instance with the specified graph name.
    /// @param gname Name of the graph to create
    explicit BaremetalGraphSimBaseImpl(const char *gname) {
        graph = std::make_shared<AIEGraph>(std::string_view(gname));
    }

    /// @brief Performs self-test and exits.
    /// @see AIEGraph::selftestexit()
    aeg_err selftestexit() override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "selftestexit");
            return aeg_err::backend_not_ready;
        }
        return graph->selftestexit();
    }

    /// @brief Enables clock for AIE columns.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::clock_enable()
    aeg_err clock_enable() {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "clock_enable");
            return aeg_err::backend_not_ready;
        }
        return graph->clock_enable();
    }

    /// @brief Disables clock for AIE columns.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::clock_disable()
    aeg_err clock_disable() {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "clock_disable");
            return aeg_err::backend_not_ready;
        }
        return graph->clock_disable();
    }

    /// @brief Sets up callback function for error handling.
    /// @param cb Pointer to the callback function
    /// @param priv Private data pointer passed to the callback function
    /// @see AIEGraph::error_handling_cb_init()
    aeg_err error_handling_cb_init(void *cb, void *priv) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "error_handling_cb_init");
            return aeg_err::backend_not_ready;
        }
        return graph->error_handling_cb_init(cb, priv);
    }

    /// @brief Initializes the graph and configures GMIOs and external buffers.
    /// @see AIEGraph::init()
    aeg_err init() override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "init");
            return aeg_err::backend_not_ready;
        }
        return graph->init();
    }

    /// @brief Applies the given CDO blobs, then initializes the graph.
    /// Forwards to AIEGraph::init() so the CDO-before-GMIO ordering is enforced
    /// in one place rather than re-implemented here.
    /// @see AIEGraph::init()
    aeg_err init(const aeg::cdo_blob *blobs, std::size_t count) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "init");
            return aeg_err::backend_not_ready;
        }
        return graph->init(blobs, count);
    }

    /// @brief Parses an in-memory CDO blob and applies it to the device.
    /// @param addr Pointer to the CDO blob in memory.
    /// @param len  Length of the CDO blob in bytes.
    /// @return aeg_err indicating success or failure
    /// @see aeg::cdo::parse_and_apply()
    aeg_err load_cdo(uintptr_t addr, size_t len) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "load_cdo");
            return aeg_err::backend_not_ready;
        }

        return graph->load_cdo(addr, len);
    }

    /// @brief Starts and runs the graph for a specified number of iterations.
    /// @param num Number of iterations to run
    /// @see AIEGraph::run()
    aeg_err run(int num) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "run");
            return aeg_err::backend_not_ready;
        }
        return graph->run(num);
    }

    /// @brief Transfers data from AIE to GMIO (non-blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::aie2gm_nb()
    aeg_err aie2gm_nb(char *name, char *data, int32_t len) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "aie2gm_nb");
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm_nb(name, data, len);
    }

    /// @brief Transfers data from GMIO to AIE (non-blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::gm2aie_nb()
    aeg_err gm2aie_nb(char *name, char *data, int32_t len) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "gm2aie_nb");
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie_nb(name, data, len);
    }

    /// @brief Transfers data from AIE to GMIO (blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::aie2gm()
    aeg_err aie2gm(char *name, char *data, int32_t len) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "aie2gm");
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm(name, data, len);
    }

    /// @brief Transfers data from GMIO to AIE (blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::gm2aie()
    aeg_err gm2aie(char *name, char *data, int32_t len) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "gm2aie");
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie(name, data, len);
    }

    /// @brief Waits for graph execution to complete.
    /// @see AIEGraph::wait()
    aeg_err wait() override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "wait");
            return aeg_err::backend_not_ready;
        }
        return graph->wait();
    }

    // External Buffer APIs
    /// @brief Sets the address for an external buffer.
    /// @param externalbuffer_name Name of the external buffer
    /// @param ptr Pointer to the address to set
    /// @see AIEGraph::setAddress()
    aeg_err setAddress(char *externalbuffer_name, void *ptr) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "setAddress");
            return aeg_err::backend_not_ready;
        }
        return graph->setAddress(externalbuffer_name, ptr);
    }

    /// @brief Waits for an external buffer operation to complete.
    /// @param externalbuffer_name Name of the external buffer
    /// @param port_name Name of the port to wait for
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::wait()
    aeg_err wait(char *externalbuffer_name, std::string_view port_name) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "wait");
            return aeg_err::backend_not_ready;
        }
        return graph->wait(externalbuffer_name, port_name);
    }

    /// @brief Transfers data from GMIO to AIE using external buffer (non-blocking).
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::gm2aie_nb()
    aeg_err gm2aie_nb(char *externalbuffer_name, std::string_view port_name) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "gm2aie_nb");
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie_nb(externalbuffer_name, port_name);
    }

    /// @brief Transfers data from AIE to GMIO using external buffer (non-blocking).
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::aie2gm_nb()
    aeg_err aie2gm_nb(char *externalbuffer_name, std::string_view port_name) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "aie2gm_nb");
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm_nb(externalbuffer_name, port_name);
    }

    /// @brief Sets up ping-pong buffers for an external buffer.
    /// @param externalbuffer_name Name of the external buffer
    /// @param ptr1 Address of the first buffer pointer
    /// @param ptr2 Address of the second buffer pointer
    /// @param sync_cache If true (default), flush CPU cache before DMA. Set to
    ///        false for DMA-to-DMA buffers where the CPU never touches the data.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::setupPingPongBuffers()
    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2, bool sync_cache = true) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "setupPingPongBuffers");
            return aeg_err::backend_not_ready;
        }
        return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2, sync_cache);
    }
    // End external Buffer API

    /// @brief Ends graph execution and performs cleanup.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::end()
    aeg_err end() {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "end");
            return aeg_err::backend_not_ready;
        }
        return graph->end();
    }

    /// @brief Waits for a specific GMIO operation to complete.
    /// @param gmio_name Name of the GMIO to wait for
    /// @param syncmode If true, waits in synchronous mode
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::gmio_wait()
    aeg_err gmio_wait(std::string_view gmio_name, bool syncmode) override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "gmio_wait");
            return aeg_err::backend_not_ready;
        }
        return graph->gmio_wait(gmio_name, syncmode);
    }

    /// @brief Allocates memory for AIE device operations.
    /// @param len Size of memory to allocate in bytes
    /// @return Pointer to allocated memory on success, nullptr on failure
    /// @see AIEGraph::malloc()
    void *malloc(uint32_t len) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "malloc");
            return nullptr;
        }
        return graph->malloc(len);
    }

    /// @brief Frees memory allocated for AIE device operations.
    /// @param addr Pointer to memory to free
    /// @see AIEGraph::free()
    aeg_err free(void *addr) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "free");
            return aeg_err::backend_not_ready;
        }
        return graph->free(addr);
    }

    /// @brief Updates data for an RTP or shared buffer port.
    /// @param port_name Name of the RTP or shared buffer port
    /// @param data Pointer to data to update
    /// @param len Number of bytes of data
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::update()
    aeg_err update(const std::string_view port_name, void *data, size_t len) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "update");
            return aeg_err::backend_not_ready;
        }
        return graph->update(port_name, data, len);
    }

    /// @brief Reads data from an RTP port.
    /// @param port_name Name of the RTP port to read from
    /// @param data Pointer to buffer to store read data
    /// @param len Number of bytes to read
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::read()
    aeg_err read(const std::string_view port_name, char *data, int32_t len) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "read");
            return aeg_err::backend_not_ready;
        }
        return graph->read(port_name, data, len);
    }

    /// @brief Dumps core log for a specific tile.
    /// @param row Row index of the tile
    /// @param col Column index of the tile
    /// @return Pointer to core log string data
    /// @see AIEGraph::dumpcorelog()
    char* dumpcorelog(int32_t row, int32_t col) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "dumpcorelog");
            return nullptr;
        }
        return graph->dumpcorelog(row, col);
    }

    /// @brief Gets the device instance pointer.
    /// @return Pointer to XAie_DevInst structure
    /// @see AIEGraph::getaiedevinst()
    void *getaiedevinst() {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "getaiedevinst");
            return nullptr;
        }
        return graph->getaiedevinst();
    }

    /// @brief Backtracks and collects error information.
    /// @return Pointer to collected error payloads
    /// @see AIEGraph::BacktrackErrors()
    void *BacktrackErrors() {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "BacktrackErrors");
            return nullptr;
        }
        return graph->BacktrackErrors();
    }

    /// @brief Debug function for setting GDB breakpoint.
    /// Placeholder function for GDB debugging support.
    void gdbpoint() {

    }

    /// @brief Starts profiling for a single port.
    /// @param port_name Name of the port to profile
    /// @param option Profiling option
    /// @param value Additional value for profiling configuration
    /// @return Handle to the profiling session
    /// @see AIEGraph::start_profiling()
    PROFILE_ID start_profiling(const std::string_view port_name, int32_t option, uint32_t value) {

        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "start_profiling");
            return -1;
        }
        return graph->start_profiling(port_name, option, value);
    }

    /// @brief Starts profiling for two ports.
    /// @param port_name1 Name of the first port to profile
    /// @param port_name2 Name of the second port to profile
    /// @param option Profiling option
    /// @param value Additional value for profiling configuration
    /// @return Handle to the profiling session
    /// @see AIEGraph::start_profiling()
    PROFILE_ID start_profiling(const std::string_view port_name1,
                               const std::string_view port_name2,
                               int32_t option,
                               uint32_t value) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "start_profiling");
            return -1;
        }
        return graph->start_profiling(port_name1, port_name2, option, value);
    }

    /// @brief Reads profiling data for a profiling session.
    /// @param h Handle to the profiling session
    /// @return Profiling data value
    /// @see AIEGraph::read_profiling()
    int64_t read_profiling(int32_t h) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "read_profiling");
            return static_cast<int64_t>(-1);
        }
        return graph->read_profiling(h);
    }

    /// @brief Stops profiling and cleans up resources.
    /// @param h Handle to the profiling session to stop
    /// @see AIEGraph::stop_profiling()
    aeg_err stop_profiling(int32_t h) {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "stop_profiling");
            return aeg_err::backend_not_ready;
        }
        return graph->stop_profiling(h);
    }

    AIEGraphSim *getGraphSimImpl() override {
        if (graph == nullptr) {
            AEG_ERROR("%s: backend not ready", "getGraphSimImpl");
            return nullptr;
        }
        return static_cast<AIEGraphSim *>(graph->getGraphImpl());
    }
    void *getGraphHandle() override { return static_cast<void *>(this); }

  private:
    std::shared_ptr<AIEGraph> graph;
};

/// @brief Creates a new BaremetalGraphSimBase instance.
/// Factory function for creating BaremetalGraphSimBaseImpl instances.
/// This function is exported as a C function for use by simulator DLLs.
/// @param gname Name of the graph to create
/// @return Pointer to newly created BaremetalGraphSimBase instance
/// @note This function is only compiled in AIE simulation mode.
extern "C" BaremetalGraphSimBase *createBaremetalGraphInstance(const char *gname) {
    return new BaremetalGraphSimBaseImpl(gname);
}
#endif

namespace aeg {
namespace detail {

#if !defined(__AIESIM__)

/// @copydoc AIEGraph::start_profiling(const std::string_view,int32_t,uint32_t) const
PROFILE_ID BasePort::start_profiling(int32_t option, uint32_t value) const {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid profiling port name", "start_profiling");
        return -1;
    }
    if (!is_valid_profiling_option(option)) {
        AEG_ERROR("%s: invalid profiling option=%d", "start_profiling", option);
        return -1;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "start_profiling");
        return -1;
    }
    return g->start_profiling(portName, option, value);
}

/// @copydoc AIEGraph::read_profiling(PROFILE_ID) const
int64_t BasePort::read_profiling(PROFILE_ID h) const {
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "read_profiling", h);
        return static_cast<int64_t>(-1);
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "read_profiling");
        return static_cast<int64_t>(-1);
    }
    return g->read_profiling(h);
}

/// @copydoc AIEGraph::stop_profiling(PROFILE_ID) const
aeg_err BasePort::stop_profiling(PROFILE_ID h) const {
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "stop_profiling", h);
        return aeg_err::invalid_state;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "stop_profiling");
        return aeg_err::backend_not_ready;
    }
    return g->stop_profiling(h);
}

/// @copydoc AIEGraph::gm2aie_nb(const char*,char*,int32_t) const
aeg_err InputGMIO::gm2aie_nb(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie_nb");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getGmioApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get GMIO API implementation", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->gm2aie_nb(data, len));
}

/// @copydoc AIEGraph::gm2aie(const char*,char*,int32_t) const
aeg_err InputGMIO::gm2aie(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getGmioApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get GMIO API implementation", "gm2aie");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->gm2aie(data, len));
}

/// @copydoc AIEGraph::aie2gm_nb(const char*,char*,int32_t) const
aeg_err OutputGMIO::aie2gm_nb(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm_nb");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getGmioApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get GMIO API implementation", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->aie2gm_nb(data, len));
}

/// @copydoc AIEGraph::aie2gm(const char*,char*,int32_t) const
aeg_err OutputGMIO::aie2gm(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getGmioApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get GMIO API implementation", "aie2gm");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->aie2gm(data, len));
}

/// @copydoc AIEGraph::gmio_wait(std::string,bool) const
aeg_err OutputGMIO::gmio_wait(bool syncmode) {
    if (!syncmode) {
        AEG_ERROR("%s: invalid syncmode argument", "gmio_wait");
        return aeg_err::user_error;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getGmioApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get GMIO API implementation", "gmio_wait");
        return aeg_err::backend_not_ready;
    }
    return static_cast<aeg_err>(apiImpl->wait(syncmode));
}

#endif // !defined(__AIESIM__)

#if !defined(__AIESIM__)
/// @copydoc AIEGraph::setAddress(const char*,void*) const
aeg_err ExternalBuffer::setAddress(void *ptr) {
    if (ptr == nullptr) {
        AEG_ERROR("%s: null argument", "setAddress");
        return aeg_err::null_argument;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getExternalBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "setAddress");
        return aeg_err::backend_not_ready;
    }
    apiImpl->setAddress(ptr);
    return aeg_err::ok;
}

/// @copydoc AIEGraph::wait(const char*,std::string) const
aeg_err ExternalBuffer::wait(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "wait");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getExternalBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "wait");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->wait(port_name));
}

/// @copydoc AIEGraph::gm2aie_nb(const char*,std::string) const
aeg_err ExternalBuffer::gm2aie_nb(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getExternalBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->gm2aie_nb(port_name));
}

/// @copydoc AIEGraph::aie2gm_nb(const char*,std::string) const
aeg_err ExternalBuffer::aie2gm_nb(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getExternalBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->aie2gm_nb(port_name));
}

/// @copydoc AIEGraph::setupPingPongBuffers(const char*,void*,void*,bool) const
aeg_err ExternalBuffer::setupPingPongBuffers(void *const ptr1, void *const ptr2, bool sync_cache) {
    if ((ptr1 == nullptr) || (ptr2 == nullptr)) {
        AEG_ERROR("%s: null argument", "setupPingPongBuffers");
        return aeg_err::null_argument;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getExternalBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "setupPingPongBuffers");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->setupPingPongBuffers(ptr1, ptr2, sync_cache));
}

/// Transfers data using the default output port `<buffer>.out[0]`.
/// @see AIEGraph::gm2aie_nb(const char*,std::string_view) const
aeg_err ExternalBuffer::gm2aie_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    std::array<char, 256> buf = {};
    std::size_t len = 0U;
    std::size_t i = 0U;
    while((i < portName.size()) && (len < (buf.size() - 1U))) {
        buf[len] = portName[i];
        ++len;
        ++i;
    }
    const char suffix[] = ".out[0]";
    i = 0U;
    while((suffix[i] != '\0') && (len < (buf.size() - 1U))) {
        buf[len] = suffix[i];
        ++len;
        ++i;
    }
    return gm2aie_nb(std::string_view(buf.data(), len));
}

/// Transfers data using the default input port `<buffer>.in[0]`.
/// @see AIEGraph::aie2gm_nb(const char*,std::string_view) const
aeg_err ExternalBuffer::aie2gm_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    std::array<char, 256> buf = {};
    std::size_t len = 0U;
    std::size_t i = 0U;
    while((i < portName.size()) && (len < (buf.size() - 1U))) {
        buf[len] = portName[i];
        ++len;
        ++i;
    }
    const char suffix[] = ".in[0]";
    i = 0U;
    while((suffix[i] != '\0') && (len < (buf.size() - 1U))) {
        buf[len] = suffix[i];
        ++len;
        ++i;
    }
    return aie2gm_nb(std::string_view(buf.data(), len));
}

/// @copydoc AIEGraph::wait(const char*,std::string_view) const
aeg_err ExternalBufferPort::wait() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "wait");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string_view bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "wait");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->wait(portName));
}

/// @copydoc AIEGraph::gm2aie_nb(const char*,std::string_view) const
aeg_err ExternalBufferPort::gm2aie_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string_view bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->gm2aie_nb(portName));
}

/// @copydoc AIEGraph::aie2gm_nb(const char*,std::string_view) const
aeg_err ExternalBufferPort::aie2gm_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string_view bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get External Buffer API implementation", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->aie2gm_nb(portName));
}

/// @copydoc AIEGraph::update(const std::string_view,void*,size_t) const
aeg_err RTP::update(void *data, size_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "update");
        return aeg_err::null_argument;
    }
    if (len == 0U) {
        AEG_ERROR("%s: invalid length argument", "update");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getRtpApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get RTP API implementation", "update");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->update(data, len));
}

/// @copydoc AIEGraph::read(const std::string_view,char*,int32_t) const
aeg_err RTP::read(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "read");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "read");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getRtpApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get RTP API implementation", "read");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->read(data, len));
}

/// @copydoc AIEGraph::update(const std::string_view,void*,size_t) const
aeg_err SharedBuffer::update(void *data, size_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "update");
        return aeg_err::null_argument;
    }
    if (len == 0U) {
        AEG_ERROR("%s: invalid length argument", "update");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        apiImpl = g->getSharedBufferApiImpl(portName);
    }
    if (!apiImpl) {
        AEG_ERROR("%s: failed to get shared buffer API implementation", "update");
        return aeg_err::backend_not_ready;
    }
    return err_from_abr(apiImpl->update(data, len));
}
#endif // !defined(__AIESIM__)

} // namespace detail
} // namespace aeg
