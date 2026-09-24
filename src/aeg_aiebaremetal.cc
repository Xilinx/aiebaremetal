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
#include <iostream>
#include <unordered_map>
#include <vector>
#ifdef DEBUG_RUNTIME
#include "hw_config.h"
#endif
// include mpu header
#ifdef ARMR5
#include "xil_mpu.h"
#endif

#include <xaiengine.h>
#include "aiebaremetal.h"
#include <sstream>
#include "aeg_class_api.h"
#include "aeg_configure.h"
#include "cert_elf_loader.h"
#include "cert_host.h"
#include "event.h"
#include "log/klogr.h"
#include "xaie_generate_events.h"
#include "xaiemem.h"
#include <unistd.h>
#ifdef __AIESIM__
#include "drivers/aiengine/xioutils.h"
#include "aiesim_base_apis.h"
#include <dlfcn.h>
#endif

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xparameters_ps.h" /* defines XPAR values */
#include "xscugic.h"
#include <xil_exception.h>
#include <xstatus.h>
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */

#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID

using namespace abr;

class AIEGraphSim;
std::vector<std::shared_ptr<AIEGraphSim>> *AIEGraphSim_List = NULL;
#include "common_layer/aeg_api_message.h"
#include "common_layer/aegothers/fal_util.h"

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

inline bool is_valid_cstr(const char *value) { return value != nullptr && value[0] != '\0'; }

inline bool is_valid_profiling_option(int option) {
    return option >= io_total_stream_running_to_idle_cycles && option <= io_stream_running_event_count;
}
} // namespace

extern unsigned char ctrl_code_elf[];
extern unsigned int ctrl_code_elf_len;

#ifdef __AIESIM__
extern void *ess_Write32_Ptr;
extern void *ess_Read32_Ptr;

extern int g_start_plio;
extern const char _binary_scsim_config_json_start[];
extern const char _binary_scsim_config_json_end[];

auto len = _binary_scsim_config_json_end - _binary_scsim_config_json_start;
std::string json_file(_binary_scsim_config_json_start, len);

/// @brief Extracts library path from SCSIM configuration JSON.
/// Parses the embedded JSON configuration to extract the library path for
/// PL IP block simulation. Combines work path with the library path.
/// @param json_file Reference to JSON configuration string
/// @return Allocated C-string containing the full library path
/// @note The returned string is allocated with new[] and should be managed by caller.
///       This function is only used in AIE simulation mode.
const char *getLibPathfromScsimConfig(std::string &json_file) {
    std::istringstream iss(json_file);
    std::ofstream outfile("temp.txt", std::ios::out);
    outfile << iss.str();
    outfile.close();

    boost::property_tree::ptree pt;
    std::string short_lib_path;

    boost::property_tree::read_json(iss, pt);
    for (auto &child : pt.get_child("SimulationConfig.pl_ip_block")) {
        short_lib_path = child.second.get<std::string>("lib_path");
    }
    const char *work_path = "./Work/"; // getenv("AIE_COMPILE_WORK");
    size_t len = std::strlen(work_path) + std::strlen(short_lib_path.c_str()) + 1;
    char *lib_path = new char[len];
    std::sprintf(lib_path, "%s%s", work_path, short_lib_path.c_str());
    return lib_path;
}

const char *lib_path = getLibPathfromScsimConfig(json_file);

/// @brief Opens a dynamic library and loads required symbols.
/// Opens the specified dynamic library using dlopen and loads the ess_Write32
/// and ess_Read32 function pointers for simulation support.
/// @param lib_path Path to the dynamic library to open
/// @throws std::runtime_error if dlopen fails
/// @note This function is only used in AIE simulation mode.
///       The function pointers are stored in global variables ess_Write32_Ptr and ess_Read32_Ptr.
static void open_dl(const char *lib_path) {
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        throw std::runtime_error(dlerror());
    }
    ess_Write32_Ptr = (void *)dlsym(handle, "ess_Write32");
    ess_Read32_Ptr = (void *)dlsym(handle, "ess_Read32");

    printf("ess_Write32_Ptr = %p\n", ess_Write32_Ptr);
    printf("ess_Read32_Ptr = %p\n", ess_Read32_Ptr);
    return;
}

#endif

extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
XScuGic xInterruptController;
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */

void abr_error_isr(void *data);
/// @brief Initializes the AIE error interrupt service routine.
/// Sets up the interrupt controller and registers the error ISR handler.
/// This function is called automatically when the first graph is created.
/// @note This function is only active on hardware (not in simulation or frontend mode).
///       It uses XScuGic interrupt controller and registers handler for AIE IRQ vector.
void abr_init_isr() {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__))
    static int init_isr = 0;
    int Ret = 0;
    XScuGic_Config *IntcConfig;
#define AIE_IRQ_VECT_ID_0 180U

    if (init_isr) {
        return;
    }
    Xil_ExceptionInit();
    XScuGic_DeviceInitialize(INTC_DEVICE_ID);

    Xil_ExceptionDisable(); /* Initialize the interrupt controller driver */
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (NULL == IntcConfig)
        return;
    Ret = XScuGic_CfgInitialize(&xInterruptController, IntcConfig, IntcConfig->CpuBaseAddress);
    if (Ret != XST_SUCCESS)
        return;
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_DeviceInterruptHandler,
                                 (void *)INTC_DEVICE_ID);

    Xil_ExceptionEnable();
    XScuGic_Connect(&xInterruptController, AIE_IRQ_VECT_ID_0, (Xil_InterruptHandler)abr_error_isr, (void *)NULL);

    XScuGic_Enable(&xInterruptController, AIE_IRQ_VECT_ID_0);

    init_isr = 1;
    return;
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */
}

/// @brief Dumps the UC (User Core) application ELF to a file.
/// Writes the embedded UC application ELF binary data to "output.bin" file.
/// This function is only active in AIE simulation mode.
/// @note This is a debug utility function used for extracting the embedded ELF binary.
void dump_uc_app() {
#ifdef __AIESIM__
    std::ofstream outFile("output.bin", std::ios::binary | std::ios::out);
    if (!outFile) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }
    // Read data from istringstream and write it to the binary file
    outFile.write(_binary___app_elf_start, _binary___app_elf_end - _binary___app_elf_start);
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
void aie_driver_test(XAie_DevInst *DevInst) {
    (void)DevInst;
}

/// @brief AIE Graph simulation class for managing graph execution.
/// This class provides the core functionality for initializing, configuring,
/// and executing AIE graphs. It manages device instances, graph APIs, event APIs,
/// and memory allocation for AIE operations.
class AIEGraphSim {
  public:
    /// @brief Destructor for AIEGraphSim.
    /// Cleans up the device instance by calling XAie_Finish().
    ~AIEGraphSim() { XAie_Finish(&DevInst); }

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
    AIEGraphSim(std::string GraphName) {
        int rpu = 0;
#ifdef ARMR5
        rpu = 1;
#endif
        struct abr::aiecompiler_options aiecompiler_options;

        auto &aiecompilerconfig = cfg.get_aiecompiler_config();
        aiecompiler_options.broadcast_enable_core = aiecompilerconfig.broadcast_enable_core;
        printf("**********Broadcast enable is %d\n", aiecompiler_options.broadcast_enable_core);
        xaiemem = std::make_shared<XAieMem>(&DevInst);
        auto &dconfig = cfg.get_driver_config();
        auto *gconfig = cfg.get_graph_config(GraphName);
        if (!gconfig) {
            AEG_ERROR("%s: graph config for '%s' not found", __func__, GraphName.c_str());
            std::abort();
            return;
        }
        GraphApi = std::make_shared<abr::graph_api>(gconfig);
        EventApi = std::make_shared<abr::event>(&cfg);
        auto aiebase = dconfig.base_address;
        // TODO: AIE compiler should provide a rpu base_address, after that change the logic, now just hardcode.
        if (rpu) {
            aiebase = RPU_AIE_BASE;
            printf("RPU aiebase = 0x%llx\n", (unsigned long long)aiebase);
        }
        XAie_SetupConfig(ConfigPtr, dconfig.hw_gen,
                         aiebase, // dconfig.base_address,
                         dconfig.column_shift, dconfig.row_shift, dconfig.num_columns, dconfig.num_rows,
                         dconfig.shim_row, dconfig.mem_row_start, dconfig.mem_num_rows, dconfig.aie_tile_row_start,
                         dconfig.aie_tile_num_rows);

        AEG_LOG("dconfig.hw_gen = %d nrows = %d ncol = %d\n", dconfig.hw_gen, dconfig.num_rows, dconfig.num_columns);

        AEG_LOG("Before setuppartitionconfig() base address :0x%llx\n", (unsigned long long)DevInst.BaseAddr);
        AieRC RC = XAie_SetupPartitionConfig(
            &DevInst, (aiebase + (dconfig.partition_overlay_start_cols[0] << dconfig.column_shift)),
            dconfig.partition_overlay_start_cols[0], dconfig.partition_num_cols);

            AEG_LOG("After setuppartitionconfig() base address :0x%llx\n", (unsigned long long)DevInst.BaseAddr);

        if (RC != XAIE_OK) {
            printf("Partition initialization failed.\n");
            return;
        }

        RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
        if (RC != XAIE_OK) {
            printf("Driver initialization failed.\n");
            return;
        }
        fal_util::initialize(&DevInst);
        abr::config_manager::initialize(&DevInst, dconfig.mem_num_rows, aiecompiler_options.broadcast_enable_core);
        this->cert.init_devinst(&this->DevInst);
        GraphApi->configure();
#ifdef __AIESIM__
#ifndef __LOCAL_TEST__
        open_dl(lib_path);
#endif
#endif
    }

    /// @brief Performs self-test and exits.
    /// Tests various functions including error handling, clock enable/disable operations.
    /// In simulation mode, updates NPI address based on device generation.
    /// @note This function is used for debugging and validation purposes.
    ///       It tests error handling initialization and clock control functions.
    aeg_err selftestexit() {
        int ret = 0;
        aeg_err status = aeg_err::ok;

#ifdef __AIESIM__
        std::cout << "aie_driver_test pid = " << getpid() << std::endl;
        XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_SIM);
        switch (DevInst.DevProp.DevGen) {
        case XAIE_DEV_GEN_AIE2PS:
        case XAIE_DEV_GEN_AIEML:
            XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
            break;
        case XAIE_DEV_GEN_AIE:
            XAie_UpdateNpiAddr(&DevInst, 0xF70A0000);
            break;
        default:
            printf("DevGen not found: %d\n", DevInst.DevProp.DevGen);
        }
#endif /* __AIESIM__ */
        ret = selftest_errorhandling();
        if (ret != 0) {
            AEG_ERROR("%s: operation failed with code=%d", __func__, ret);
            return static_cast<aeg_err>(ret);
        }

        status = clock_enable();
        if (status != aeg_err::ok) {
            printf("Clock enable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", __func__, static_cast<int>(status));
            return status;
        } else {
            printf("Clock enabled.\n");
        }
        status = clock_disable();
        if (status != aeg_err::ok) {
            printf("Clock disable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", __func__, static_cast<int>(status));
            return status;
        } else {
            printf("Clock disabled.\n");
        }
        status = clock_enable();
        if (status != aeg_err::ok) {
            printf("Clock enable failed\n");
            AEG_ERROR("%s: operation failed with status=%d", __func__, static_cast<int>(status));
            return status;
        } else {
            printf("Clock enabled.\n");
        }

        return aeg_err::ok;
    }

    /// @brief Enables clock for AIE columns.
    /// Enables the clock for all columns in the device partition.
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err clock_enable() {
        AieRC rc = XAie_PmSetColumnClk(&DevInst, DevInst.StartCol, DevInst.NumCols, 1);
        if (rc != XAIE_OK) {
            AEG_ERROR("%s: XAie_PmSetColumnClk enable failed rc=%d", __func__, static_cast<int>(rc));
            return aeg_err::aie_driver_error;
        }
        return aeg_err::ok;
    }

    /// @brief Disables clock for AIE columns.
    /// Disables the clock for all columns in the device partition.
    /// @return aeg_err indicating success (0) or failure (non-zero)
    aeg_err clock_disable() {
        AieRC rc = XAie_PmSetColumnClk(&DevInst, DevInst.StartCol, DevInst.NumCols, 0);
        if (rc != XAIE_OK) {
            AEG_ERROR("%s: XAie_PmSetColumnClk disable failed rc=%d", __func__, static_cast<int>(rc));
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
        if (cb == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        error_handling_cb = (void (*)(std::vector<XAie_ErrorPayload>, void *))cb;
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
#ifdef ARMR5
        Xil_SetMPURegion(RPU_AIE_BASE, 0x40000000, DEVICE_NONSHARED | PRIV_RW_USER_RW);
#endif
        if (!xaiemem) {
            AEG_ERROR("%s: backend memory manager not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        if (!GraphApi) {
            AEG_ERROR("%s: graph API backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }

        AEG_LOG("init----" << std::endl);
        aeg_err status = cfg.ConfigureGmios(xaiemem);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureGmios failed status=%d", __func__, static_cast<int>(status));
            return status;
        }
        status = cfg.ConfigureExternalBuffers(xaiemem);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureExternalBuffers failed status=%d", __func__, static_cast<int>(status));
            return status;
        }
        status = cfg.ConfigureRtps(GraphApi);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureRtps failed status=%d", __func__, static_cast<int>(status));
            return status;
        }
        status = cfg.ConfigureSharedBuffers(GraphApi);
        if (status != aeg_err::ok) {
            if (status == aeg_err::null_argument) {
                status = aeg_err::backend_not_ready;
            }
            AEG_ERROR("%s: ConfigureSharedBuffers failed status=%d", __func__, static_cast<int>(status));
            return status;
        }

        if (!xaiemem || !GraphApi) {
            AEG_ERROR("%s: postcondition failed; backend became unavailable after init", __func__);
            return aeg_err::postcondition_failed;
        }
        return aeg_err::ok;
    }

    /// @brief Starts and runs the graph for a specified number of iterations.
    /// Executes the graph for the given number of iterations. In simulation mode,
    /// also starts PLIOs for data transfer.
    /// @param num Number of iterations to run
    /// @note In DEBUG_RUNTIME mode, additional debug code may be executed.
    aeg_err run(int num) {
        // gdb_point(1);

#ifdef __AIESIM__
        aiesim::StartPlios();
#endif

        if (!GraphApi) {
            AEG_ERROR("%s: backend not ready", __func__);
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
            std::cout << "enable core core status is " << d2 << " clock is " << gate << "before gate" << g0
                      << std::endl;
            int loop = 0;
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
    aeg_err aie2gm_nb(char *gmio_name, char *data, int len) {
        if (!is_valid_cstr(gmio_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len <= 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (!gmio_api_impl) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", __func__, gmio_name);
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
    aeg_err aie2gm(char *gmio_name, char *data, int len) {
        if (!is_valid_cstr(gmio_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len <= 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (!gmio_api_impl) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", __func__, gmio_name);
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
        int channel = 0, col = 12, row = 0;
        XAie_LocType loc = XAie_TileLoc(12, 0);
        const uint32_t col_12_row_0 = (loc.Col << XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
        uint32_t mm2sc0_status = 0x00009328;
        uint32_t d1;
        int n = 1;

        while (n-- >= 0) {
            XAie_Read32(&DevInst, col_12_row_0 + mm2sc0_status, &d1);
            std::cout << "mm2s status " << d1 << std::endl;
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
    aeg_err gm2aie_nb(char *gmio_name, char *data, int len) {
        if (!is_valid_cstr(gmio_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len <= 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
        if (!gmio_api_impl) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", __func__, gmio_name);
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
    aeg_err gm2aie(char *gmio_name, char *data, int len) {
        if (!is_valid_cstr(gmio_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len <= 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name); // grab gmio api impl based on gmio name
        if (!gmio_api_impl) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", __func__, gmio_name);
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
    aeg_err setAddress(char *externalbuffer_name, void *ptr) {
        if (!is_valid_cstr(externalbuffer_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (ptr == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (!externalbuffer_api_impl) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", __func__, externalbuffer_name);
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
    aeg_err wait(char *externalbuffer_name, std::string port_name) {
        if (!is_valid_cstr(externalbuffer_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (!externalbuffer_api_impl) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", __func__, externalbuffer_name);
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
    aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name) {
        if (!is_valid_cstr(externalbuffer_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (!externalbuffer_api_impl) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", __func__, externalbuffer_name);
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
    aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name) {
        if (!is_valid_cstr(externalbuffer_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (!externalbuffer_api_impl) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", __func__, externalbuffer_name);
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
    /// @return aeg_err indicating success or failure
    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2) {
        if (!is_valid_cstr(externalbuffer_name)) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (ptr1 == nullptr || ptr2 == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
        if (!externalbuffer_api_impl) {
            AEG_ERROR("%s: external buffer API impl for '%s' not found", __func__, externalbuffer_name);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(externalbuffer_api_impl->setupPingPongBuffers(ptr1, ptr2));
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
    aeg_err update(const std::string &port_name, void *data, size_t len) {
        if (!GraphApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len == 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        if (cfg.isRTPorSharedBuffer(port_name) == 0) { // RTP
            auto *rt_config = cfg.get_rtp_config(port_name);
            if (!rt_config) {
                AEG_ERROR("%s: RTP config for '%s' not found", __func__, port_name.c_str());
                return aeg_err::invalid_name;
            }
            return err_from_abr(GraphApi->update(rt_config, data, len));
        } else {
            auto *config = cfg.get_shared_buffer_config(port_name);
            if (!config) {
                AEG_ERROR("%s: shared buffer config for '%s' not found", __func__, port_name.c_str());
                return aeg_err::invalid_name;
            }
            return err_from_abr(GraphApi->update(config, data, len));
        }
    }

    /// @brief Reads data from an RTP port.
    /// Reads runtime parameter data from the specified RTP port. Performs error
    /// checking and reads data based on the RTP configuration name.
    /// @param port_name Name of the RTP port to read from
    /// @param data Pointer to buffer to store read data
    /// @param len Number of bytes to read
    /// @return aeg_err indicating success or failure
    aeg_err read(const std::string &port_name, char *data, int len) {
        if (!GraphApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid name argument", __func__);
            return aeg_err::invalid_name;
        }
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", __func__);
            return aeg_err::null_argument;
        }
        if (len <= 0) {
            AEG_ERROR("%s: invalid length argument", __func__);
            return aeg_err::invalid_length;
        }
        auto *rt_config = cfg.get_rtp_config(port_name);
        if (!rt_config) {
            AEG_ERROR("%s: RTP config for '%s' not found", __func__, port_name.c_str());
            return aeg_err::invalid_name;
        }
        return err_from_abr(GraphApi->read(rt_config, data, len));
    }

    /// @brief Dumps core log for a specific tile.
    /// Retrieves and returns the core log data for the specified tile location.
    /// @param row Row index of the tile
    /// @param col Column index of the tile
    /// @return Pointer to core log string data
    char *dumpcorelog(int row, int col) { return rlog(row, col, (uint64_t)&DevInst); }

    /// @brief Gets the device instance pointer.
    /// Returns a pointer to the underlying XAie_DevInst structure for direct
    /// access to low-level driver functions.
    /// @return Pointer to XAie_DevInst structure
    void *getaiedevinst() { return (void *)&DevInst; }

    /// @brief Waits for graph execution to complete.
    /// Waits for all cores in the graph to finish executing their tasks.
    /// @note This function blocks until all cores have completed execution.
    aeg_err wait() {
        if (!GraphApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return err_from_abr(GraphApi->wait());
    }

    /// @brief Ends graph execution and performs cleanup.
    /// Verifies that the graph is properly configured, waits for completion,
    /// checks that all cores finished running, and performs cleanup operations.
    /// @return aeg_err indicating success or failure
    aeg_err end() {
        AEG_LOG("graph end() called" << std::endl);
        if (!GraphApi) {
            AEG_ERROR("%s: backend not ready", __func__);
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
    aeg_err gmio_wait(std::string gmio_name, bool syncmode) {
        if (gmio_name.empty()) {
            AEG_ERROR("%s: invalid GMIO name", __func__);
            return aeg_err::invalid_name;
        }
        auto gmioApiImpl = cfg.get_gmio_api_impl(gmio_name);
        if (!gmioApiImpl) {
            AEG_ERROR("%s: GMIO API impl for '%s' not found", __func__, gmio_name.c_str());
            return aeg_err::backend_not_ready;
        }
        int wait_status = gmioApiImpl->wait(syncmode);
        if (wait_status != 0) {
            AEG_ERROR("%s: GMIO wait failed for '%s' with code=%d", __func__, gmio_name.c_str(), wait_status);
        }
        return static_cast<aeg_err>(wait_status);
    }

    /// @brief Allocates memory for AIE device operations.
    /// Allocates memory using the XAieMem wrapper class for AIE device operations.
    /// @param len Size of memory to allocate in bytes
    /// @return Pointer to allocated memory on success, nullptr on failure
    void *malloc(uint32_t len) {
        if (!xaiemem) {
            AEG_ERROR("%s: XAieMem not found", __func__);
            return nullptr;
        }
        return xaiemem->malloc(len);
    }

    /// @brief Frees memory allocated for AIE device operations.
    /// Frees memory that was previously allocated using malloc().
    /// @param addr Pointer to memory to free (must be allocated by malloc())
    aeg_err free(void *addr) {
        if (!xaiemem) {
            AEG_ERROR("%s: XAieMem not found", __func__);
            return aeg_err::backend_not_ready;
        }
        xaiemem->free(addr);
        return aeg_err::ok;
    }

    /// @brief Backtracks and collects error information.
    /// Processes error data from the device and collects error payloads.
    /// If an error handling callback is registered, it will be called with the errors.
    /// @return Pointer to collected error payloads vector
    void *BacktrackErrors() {
        backtrack_payload_cache.clear();
        if (DevInst.NumCols == 0 || DevInst.StartCol < 0 || DevInst.StartCol >= DevInst.NumCols) {
            AEG_ERROR("%s: invalid device state (StartCol=%d, NumCols=%d)", __func__, DevInst.StartCol,
                      DevInst.NumCols);
            return nullptr;
        }

        XAie_ErrorMetaData MData = {};
        constexpr u32 kPayloadSize = 32U;
        constexpr u32 kMaxBacktrackIterations = 16384U;
        XAie_ErrorPayload tmp_payload[kPayloadSize] = {};
        MData.Payload = tmp_payload;
        MData.ArraySize = kPayloadSize;
        MData.Cols.Start = DevInst.StartCol;
        MData.Cols.Num = DevInst.NumCols;
        u32 iteration = 0U;

        do {
            iteration++;
            if (iteration > kMaxBacktrackIterations) {
                AEG_ERROR("%s: exceeded max backtrack iterations (%u)", __func__, kMaxBacktrackIterations);
                return nullptr;
            }

            AieRC rc = XAie_BacktrackErrorInterrupts(&DevInst, &MData);
            if (rc != XAIE_OK && rc != XAIE_INSUFFICIENT_BUFFER_SIZE) {
                AEG_ERROR("%s: XAie_BacktrackErrorInterrupts failed rc=%d", __func__, static_cast<int>(rc));
                return nullptr;
            }

            u32 safe_error_count = static_cast<u32>(MData.ErrorCount);
            if (safe_error_count > kPayloadSize) {
                AEG_ERROR("%s: error count %u exceeds payload buffer %u; clamping", __func__, safe_error_count,
                          kPayloadSize);
                safe_error_count = kPayloadSize;
            }
            for (u32 i = 0; i < safe_error_count; i++) {
                XAie_ErrorPayload *p = &tmp_payload[i];
                backtrack_payload_cache.push_back(*p);
            }

        } while (MData.IsNextInfoValid);
        if (error_handling_cb) {
            error_handling_cb(backtrack_payload_cache, error_handling_cb_priv);
        }
        if (backtrack_payload_cache.empty()) {
            return nullptr;
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
    PROFILE_ID start_profiling(const std::string &port_name, int option, uint32_t value) {
        if (port_name.empty()) {
            AEG_ERROR("%s: invalid profiling port name", __func__);
            return -1;
        }
        if (!is_valid_profiling_option(option)) {
            AEG_ERROR("%s: invalid profiling option=%d", __func__, option);
            return -1;
        }
        if (!EventApi) {
            AEG_ERROR("%s: backend not ready", __func__);
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
    PROFILE_ID start_profiling(const std::string &port_name1, const std::string &port_name2, int option,
                               uint32_t value) {
        if (port_name1.empty() || port_name2.empty()) {
            AEG_ERROR("%s: invalid profiling port name(s)", __func__);
            return -1;
        }
        if (!is_valid_profiling_option(option)) {
            AEG_ERROR("%s: invalid profiling option=%d", __func__, option);
            return -1;
        }
        if (!EventApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return -1;
        }
        return EventApi->start_profiling(port_name1, port_name2, option, value);
    }

    /// @brief Reads profiling data for a profiling session.
    /// Retrieves the profiling data collected for the specified profiling handle.
    /// @param h Handle to the profiling session (returned by start_profiling)
    /// @return Profiling data value (typically cycle count or event count)
    long long read_profiling(PROFILE_ID h) {
        if (h < 0) {
            AEG_ERROR("%s: invalid profiling handle=%d", __func__, h);
            return -1;
        }
        if (!EventApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return -1;
        }
        return EventApi->read_profiling(h);
    }

    /// @brief Stops profiling and cleans up resources.
    /// Stops the profiling session and cleans up performance monitoring and
    /// event-related resources.
    /// @param h Handle to the profiling session to stop
    aeg_err stop_profiling(PROFILE_ID h) {
        if (h < 0) {
            AEG_ERROR("%s: invalid profiling handle=%d", __func__, h);
            return aeg_err::invalid_state;
        }
        if (!EventApi) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        EventApi->stop_profiling(h);
        return aeg_err::ok;
    }

    std::shared_ptr<GmioApiImpl> getGmioApiImpl(const std::string &name) { return cfg.get_gmio_api_impl(name); }

    std::shared_ptr<ExternalBufferApiImpl> getExternalBufferApiImpl(const std::string &name) {
        return cfg.get_externalbuffer_api_impl(name);
    }

    std::shared_ptr<RtpApiImpl> getRtpApiImpl(const std::string &name) { return cfg.get_rtp_api_impl(name); }

    std::shared_ptr<SharedBufferApiImpl> getSharedBufferApiImpl(const std::string &name) {
        return cfg.get_sharedbuffer_api_impl(name);
    }

  private:
    ConfigureParser cfg;
    XAie_DevInst DevInst = {};
    std::shared_ptr<abr::graph_api> GraphApi;
    std::shared_ptr<abr::event> EventApi;
    std::shared_ptr<XAieMem> xaiemem;
    void (*error_handling_cb)(std::vector<XAie_ErrorPayload> payloads, void *priv) = NULL;
    void *error_handling_cb_priv = NULL;
    std::vector<XAie_ErrorPayload> backtrack_payload_cache;
    cert_host::cert_api cert;

    /// @brief Self-test function for CERT (Control Engine Runtime).
    /// Tests CERT functionality by loading CERT, waking it up, initializing handshake,
    /// and submitting a test command. Used for validation and debugging.
    /// @return 0 on success, error code on failure
    int selftest_cert() {
        int ret;
        std::map<std::string_view, uint64_t> symbols;

        printf("test cert start.\n");
        ret = this->cert.load_cert(0);
        if (ret) {
            printf("load cert failed: %d\n", ret);
        }
        ret = this->cert.wakeup_cert(0);
        if (ret) {
            printf("Wakup cert col 0 failed\n");
        }
        cert.init_handshake();
        ret = cert.submit_test_command();

        if (ret) {
            printf("CERT: hsa test failed: %d\n", ret);
        } else {
            printf("CERT: hsa test PASSED.\n");
        }
        return ret;
    }

    /// @brief Self-test function for error handling.
    /// Tests error handling functionality by initializing error handling,
    /// generating error events on all columns, and verifying error detection.
    /// Updates NPI address based on device generation for proper operation.
    /// @return 0 on success, error code on failure
    int selftest_errorhandling() {
        int ret = 0;
        AieRC RC;
        std::vector<XAie_ErrorPayload> payloads;

        switch (DevInst.DevProp.DevGen) {
        case XAIE_DEV_GEN_AIE2PS:
        case XAIE_DEV_GEN_AIEML:
            XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
            break;
        case XAIE_DEV_GEN_AIE:
            XAie_UpdateNpiAddr(&DevInst, 0xF70A0000);
            break;
        default:
            printf("DevGen not found: %d\n", DevInst.DevProp.DevGen);
        }
        RC = XAie_PmRequestTiles(&DevInst, NULL, 0);
        if (RC == XAIE_OK) {
            printf("XAie_PmRequestTiles: OK\n");
        } else {
            printf("XAie_PmRequestTiles: %d\n", static_cast<int>(RC));
            return static_cast<int>(RC);
        }

        RC = XAie_ErrorHandlingInit(&DevInst);
        if (RC != XAIE_OK) {
            printf("Errorhandling init failed: %d\n", static_cast<int>(RC));
            return static_cast<int>(RC);
        }
        for (int i = DevInst.StartCol; i < DevInst.StartCol + DevInst.NumCols; i++) {
            generate_events_on_col(&DevInst, i);
        }

        printf("Finished Generating ErrorEvent\n");
        return ret;
    }
};

struct AIEGraphImpl : public AIEGraphSim {
    using AIEGraphSim::AIEGraphSim;
};

/// @brief Allocates memory for AIE device operations.
/// Wrapper function that allocates memory using the underlying graph's memory allocator.
/// @param len Size of memory to allocate in bytes
/// @return Pointer to allocated memory on success, nullptr on failure
/// @see AIEGraphSim::malloc()
void *AIEGraph::malloc(size_t len) {
    //if (!CHECK_INITIALIZED())
    //    return nullptr;
    if (len == 0) {
        printf("%s: invalid length argument\n", __func__);
        return nullptr;
    }    
    if (!graph) {
        AEG_ERROR("%s: backend not ready graph is nullptr", __func__);
        return nullptr;
    }

    return graph->malloc(len);
}

/// @brief Frees memory allocated for AIE device operations.
/// Wrapper function that frees memory using the underlying graph's memory allocator.
/// @param mem Pointer to memory to free (must be allocated by malloc())
/// @see AIEGraphSim::free()
aeg_err AIEGraph::free(void *mem) {
    //if (!CHECK_INITIALIZED()) {
    //    AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
    //    return aeg_err::invalid_state;
    //}
    if (mem == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->free(mem);
}
/**
 * @brief	Creates a new Baremetal Graph object.
 * @param	GraphName		Name of graph.
 */
AIEGraph::AIEGraph(std::string GraphName) : isInitialized(false) {
    graph = std::make_shared<AIEGraphImpl>(GraphName);

    if (!AIEGraphSim_List) {
        AIEGraphSim_List = new (std::vector<std::shared_ptr<AIEGraphSim>>)();
        AIEGraphSim_List->push_back(graph);
        abr_init_isr();
    } else {
        AIEGraphSim_List->push_back(graph);
    }
}

/// @brief Destructor for AIEGraph.
/// Cleans up the graph instance when the object is destroyed.
AIEGraph::~AIEGraph() {}

/// @brief Verifies AIEGraph has been initialized.
/// Checks internal initialized state and emits a user-facing error message when API
/// calls are made before init().
/// @param function_name Caller name used in the emitted error message
/// @return true when initialized, false otherwise
bool AIEGraph::check_initialized_impl(const char *function_name) {
    if (!isInitialized) {
        AEG_ERROR("AIEGraph::%s: invalid state (graph not initialized; call init() first)", function_name);
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
aeg_err AIEGraph::selftestexit() {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->selftestexit();
}

/// @brief Enables clock for AIE columns.
/// Enables the clock for all columns in the device partition.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::clock_enable()
aeg_err AIEGraph::clock_enable() {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->clock_enable();
}

/// @brief Disables clock for AIE columns.
/// Disables the clock for all columns in the device partition.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::clock_disable()
aeg_err AIEGraph::clock_disable() {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->clock_disable();
}

/// @brief Sets up callback function for error handling.
/// Registers a callback function that will be called when errors are detected.
/// @param cb Pointer to the callback function
/// @param priv Private data pointer passed to the callback function
/// @see AIEGraphSim::error_handling_cb_init()
aeg_err AIEGraph::error_handling_cb_init(void *cb, void *priv) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (cb == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->error_handling_cb_init(cb, priv);
}

/// @brief Initializes the graph and configures GMIOs and external buffers.
/// Sets up GMIOs and external buffers for data transfer. This must be called
/// before running the graph.
/// @see AIEGraphSim::init()
aeg_err AIEGraph::init() {
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    aeg_err status = graph->init();
    if (status != aeg_err::ok) {
        return status;
    }
    isInitialized = true;
    return aeg_err::ok;
}

/// @brief Starts and runs the graph for a specified number of iterations.
/// Executes the graph for the given number of iterations. In simulation mode,
/// also starts PLIOs for data transfer.
/// @param num Number of iterations to run
/// @see AIEGraphSim::run()
aeg_err AIEGraph::run(int num) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (num == 0 || num < -1) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
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
aeg_err AIEGraph::aie2gm_nb(char *name, char *data, int len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
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
aeg_err AIEGraph::gm2aie_nb(char *name, char *data, int len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
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
aeg_err AIEGraph::aie2gm(char *name, char *data, int len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
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
aeg_err AIEGraph::gm2aie(char *name, char *data, int len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->gm2aie(name, data, len);
}

/// @brief Waits for graph execution to complete.
/// Waits for all cores in the graph to finish executing their tasks.
/// @see AIEGraphSim::wait()
aeg_err AIEGraph::wait() {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->wait();
}

/// @brief Ends graph execution and performs cleanup.
/// Verifies that the graph is properly configured, waits for completion,
/// checks that all cores finished running, and performs cleanup operations.
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::end()
aeg_err AIEGraph::end() {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->end();
}

/// @brief Waits for a specific GMIO operation to complete.
/// Waits for a GMIO transfer operation to complete, optionally in sync mode.
/// @param gmio_name Name of the GMIO to wait for
/// @param syncmode If true, waits in synchronous mode
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::gmio_wait()
aeg_err AIEGraph::gmio_wait(std::string gmio_name, bool syncmode) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (gmio_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
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
aeg_err AIEGraph::update(const std::string &port_name, void *data, size_t len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len == 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->update(port_name, data, len);
}

/// @brief Dumps core log for a specific tile.
/// Retrieves and returns the core log data for the specified tile location.
/// @param row Row index of the tile
/// @param col Column index of the tile
/// @return Pointer to core log string data
/// @see AIEGraphSim::dumpcorelog()
char *AIEGraph::dumpcorelog(int row, int col) {
    if (!CHECK_INITIALIZED())
        return nullptr;
    if (row < 0 || col < 0)
        return nullptr;

    return graph->dumpcorelog(row, col);
}

/// @brief Gets the device instance pointer.
/// Returns a pointer to the underlying XAie_DevInst structure for direct
/// access to low-level driver functions.
/// @return Pointer to XAie_DevInst structure
/// @see AIEGraphSim::getaiedevinst()
void *AIEGraph::getaiedevinst() {
    if (!CHECK_INITIALIZED())
        return nullptr;

    return graph->getaiedevinst();
}
/// @brief Backtracks and collects error information.
/// Processes error data from the device and collects error payloads.
/// If an error handling callback is registered, it will be called with the errors.
/// @return Pointer to collected error payloads
/// @see AIEGraphSim::BacktrackErrors()
void *AIEGraph::BacktrackErrors() {
    if (!CHECK_INITIALIZED())
        return nullptr;

    return graph->BacktrackErrors();
}

/// @brief Reads data from an RTP port.
/// Reads runtime parameter data from the specified RTP port.
/// @param port_name Name of the RTP port to read from
/// @param data Pointer to buffer to store read data
/// @param len Number of bytes to read
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::read()
aeg_err AIEGraph::read(const std::string &port_name, char *data, int len) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", __func__);
        return aeg_err::invalid_length;
    }
    if (!graph) {
        AEG_ERROR("%s: backend not ready", __func__);
        return aeg_err::backend_not_ready;
    }
    return graph->read(port_name, data, len);
}

// External Buffer APIs
/// @brief Sets the address for an external buffer.
/// Sets the address pointer for an external buffer used in data transfer operations.
/// @param externalbuffer_name Name of the external buffer
/// @param ptr Pointer to the address to set
/// @see AIEGraphSim::setAddress()
aeg_err AIEGraph::setAddress(char *externalbuffer_name, void *ptr) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (ptr == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
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
aeg_err AIEGraph::wait(char *externalbuffer_name, std::string port_name) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
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
aeg_err AIEGraph::gm2aie_nb(char *externalbuffer_name, std::string port_name) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
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
aeg_err AIEGraph::aie2gm_nb(char *externalbuffer_name, std::string port_name) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name) || port_name.empty()) {
        AEG_ERROR("%s: invalid name argument", __func__);
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
/// @return aeg_err indicating success or failure
/// @see AIEGraphSim::setupPingPongBuffers()
aeg_err AIEGraph::setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (!is_valid_cstr(externalbuffer_name)) {
        AEG_ERROR("%s: invalid name argument", __func__);
        return aeg_err::invalid_name;
    }
    if (ptr1 == nullptr || ptr2 == nullptr) {
        AEG_ERROR("%s: null argument", __func__);
        return aeg_err::null_argument;
    }
    return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
}
// End external Buffer API

/// @brief Starts profiling for a single port.
/// Starts performance profiling for a specified port with the given option and value.
/// @param port_name Name of the port to profile
/// @param option Profiling option (e.g., stream running cycles, event count)
/// @param value Additional value for profiling configuration
/// @return Handle to the profiling session
/// @see AIEGraphSim::start_profiling()
int AIEGraph::start_profiling(const std::string &port_name, int option, uint32_t value) {
    if (!CHECK_INITIALIZED())
        return -1;
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid profiling port name", __func__);
        return -1;
    }
    if (!is_valid_profiling_option(option)) {
        AEG_ERROR("%s: invalid profiling option=%d", __func__, option);
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
int AIEGraph::start_profiling(const std::string &port_name1, const std::string &port_name2, int option,
                              uint32_t value) {
    if (!CHECK_INITIALIZED())
        return -1;
    if (port_name1.empty() || port_name2.empty()) {
        AEG_ERROR("%s: invalid profiling port name(s)", __func__);
        return -1;
    }
    if (!is_valid_profiling_option(option)) {
        AEG_ERROR("%s: invalid profiling option=%d", __func__, option);
        return -1;
    }

    return graph->start_profiling(port_name1, port_name2, option, value);
}

/// @brief Reads profiling data for a profiling session.
/// Retrieves the profiling data collected for the specified profiling handle.
/// @param h Handle to the profiling session (returned by start_profiling)
/// @return Profiling data value (typically cycle count or event count)
/// @see AIEGraphSim::read_profiling()
long long AIEGraph::read_profiling(int h) {
    if (!CHECK_INITIALIZED())
        return -1;
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", __func__, h);
        return -1;
    }

    return graph->read_profiling(h);
}

/// @brief Stops profiling and cleans up resources.
/// Stops the profiling session and cleans up performance monitoring resources.
/// @param h Handle to the profiling session to stop
/// @see AIEGraphSim::stop_profiling()
aeg_err AIEGraph::stop_profiling(int h) {
    if (!CHECK_INITIALIZED()) {
        AEG_ERROR("%s: invalid state (graph not initialized)", __func__);
        return aeg_err::invalid_state;
    }
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", __func__, h);
        return aeg_err::invalid_state;
    }
    return graph->stop_profiling(h);
}

/// @brief Error interrupt service routine.
/// Handles AIE error interrupts by disabling interrupts, backtracking errors
/// for all graphs, and re-enabling interrupts. This is called automatically
/// by the interrupt controller when an AIE error occurs.
/// @param data Unused parameter (required by interrupt handler signature)
/// @note This function is only active on hardware (not in simulation).
void abr_error_isr(void *data) {
    (void)data;
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__))
    XScuGic_Disable(&xInterruptController, AIE_IRQ_VECT_ID_0);

    for (std::shared_ptr<AIEGraphSim> graph : *AIEGraphSim_List) {
        (void)graph->BacktrackErrors();
    }
    XScuGic_Enable(&xInterruptController, AIE_IRQ_VECT_ID_0);
#endif /* !(defined(__AIESIM__) || defined(__ADF_FRONTEND__)) */
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
    BaremetalGraphSimBaseImpl(const char *gname) {
        std::string strGame(gname);
        graph = std::make_shared<AIEGraph>(strGame);
    }

    /// @brief Performs self-test and exits.
    /// @see AIEGraph::selftestexit()
    aeg_err selftestexit() override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->selftestexit();
    }

    /// @brief Enables clock for AIE columns.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::clock_enable()
    aeg_err clock_enable() {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->clock_enable();
    }

    /// @brief Disables clock for AIE columns.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::clock_disable()
    aeg_err clock_disable() {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->clock_disable();
    }

    /// @brief Sets up callback function for error handling.
    /// @param cb Pointer to the callback function
    /// @param priv Private data pointer passed to the callback function
    /// @see AIEGraph::error_handling_cb_init()
    aeg_err error_handling_cb_init(void *cb, void *priv) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->error_handling_cb_init(cb, priv);
    }

    /// @brief Initializes the graph and configures GMIOs and external buffers.
    /// @see AIEGraph::init()
    aeg_err init() override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->init();
    }

    /// @brief Starts and runs the graph for a specified number of iterations.
    /// @param num Number of iterations to run
    /// @see AIEGraph::run()
    aeg_err run(int num) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->run(num);
    }

    /// @brief Transfers data from AIE to GMIO (non-blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::aie2gm_nb()
    aeg_err aie2gm_nb(char *name, char *data, int len) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm_nb(name, data, len);
    }

    /// @brief Transfers data from GMIO to AIE (non-blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::gm2aie_nb()
    aeg_err gm2aie_nb(char *name, char *data, int len) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie_nb(name, data, len);
    }

    /// @brief Transfers data from AIE to GMIO (blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::aie2gm()
    aeg_err aie2gm(char *name, char *data, int len) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm(name, data, len);
    }

    /// @brief Transfers data from GMIO to AIE (blocking).
    /// @param name Name of the GMIO to use for transfer
    /// @param data Pointer to data buffer to transfer
    /// @param len Length of data in bytes
    /// @see AIEGraph::gm2aie()
    aeg_err gm2aie(char *name, char *data, int len) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie(name, data, len);
    }

    /// @brief Waits for graph execution to complete.
    /// @see AIEGraph::wait()
    aeg_err wait() override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
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
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->setAddress(externalbuffer_name, ptr);
    }

    /// @brief Waits for an external buffer operation to complete.
    /// @param externalbuffer_name Name of the external buffer
    /// @param port_name Name of the port to wait for
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::wait()
    aeg_err wait(char *externalbuffer_name, std::string port_name) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->wait(externalbuffer_name, port_name);
    }

    /// @brief Transfers data from GMIO to AIE using external buffer (non-blocking).
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::gm2aie_nb()
    aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->gm2aie_nb(externalbuffer_name, port_name);
    }

    /// @brief Transfers data from AIE to GMIO using external buffer (non-blocking).
    /// @param externalbuffer_name Name of the external buffer to use
    /// @param port_name Name of the port for the transfer
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::aie2gm_nb()
    aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->aie2gm_nb(externalbuffer_name, port_name);
    }

    /// @brief Sets up ping-pong buffers for an external buffer.
    /// @param externalbuffer_name Name of the external buffer
    /// @param ptr1 Address of the first buffer pointer
    /// @param ptr2 Address of the second buffer pointer
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::setupPingPongBuffers()
    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
    }
    // End external Buffer API

    /// @brief Ends graph execution and performs cleanup.
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::end()
    aeg_err end() {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->end();
    }

    /// @brief Waits for a specific GMIO operation to complete.
    /// @param gmio_name Name of the GMIO to wait for
    /// @param syncmode If true, waits in synchronous mode
    /// @return aeg_err indicating success or failure
    /// @see AIEGraph::gmio_wait()
    aeg_err gmio_wait(std::string gmio_name, bool syncmode) override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->gmio_wait(gmio_name, syncmode);
    }

    /// @brief Allocates memory for AIE device operations.
    /// @param len Size of memory to allocate in bytes
    /// @return Pointer to allocated memory on success, nullptr on failure
    /// @see AIEGraph::malloc()
    void *malloc(uint32_t len) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return nullptr;
        }
        return graph->malloc(len);
    }

    /// @brief Frees memory allocated for AIE device operations.
    /// @param addr Pointer to memory to free
    /// @see AIEGraph::free()
    aeg_err free(void *addr) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
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
    aeg_err update(const std::string &port_name, void *data, size_t len) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
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
    aeg_err read(const std::string &port_name, char *data, int len) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->read(port_name, data, len);
    }

    /// @brief Dumps core log for a specific tile.
    /// @param row Row index of the tile
    /// @param col Column index of the tile
    /// @return Pointer to core log string data
    /// @see AIEGraph::dumpcorelog()
    char *dumpcorelog(int row, int col) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return nullptr;
        }
        return graph->dumpcorelog(row, col);
    }

    /// @brief Gets the device instance pointer.
    /// @return Pointer to XAie_DevInst structure
    /// @see AIEGraph::getaiedevinst()
    void *getaiedevinst() {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return nullptr;
        }
        return graph->getaiedevinst();
    }

    /// @brief Backtracks and collects error information.
    /// @return Pointer to collected error payloads
    /// @see AIEGraph::BacktrackErrors()
    void *BacktrackErrors() {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return nullptr;
        }
        return graph->BacktrackErrors();
    }

    /// @brief Debug function for setting GDB breakpoint.
    /// Placeholder function for GDB debugging support.
    void gdbpoint() {
        // gdb_point(1);
    }

    /// @brief Starts profiling for a single port.
    /// @param port_name Name of the port to profile
    /// @param option Profiling option
    /// @param value Additional value for profiling configuration
    /// @return Handle to the profiling session
    /// @see AIEGraph::start_profiling()
    int start_profiling(const std::string &port_name, int option, uint32_t value) {
        // gdbpoint();
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
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
    int start_profiling(const std::string &port_name1, const std::string &port_name2, int option, uint32_t value) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return -1;
        }
        return graph->start_profiling(port_name1, port_name2, option, value);
    }

    /// @brief Reads profiling data for a profiling session.
    /// @param h Handle to the profiling session
    /// @return Profiling data value
    /// @see AIEGraph::read_profiling()
    long long read_profiling(int h) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return -1;
        }
        return graph->read_profiling(h);
    }

    /// @brief Stops profiling and cleans up resources.
    /// @param h Handle to the profiling session to stop
    /// @see AIEGraph::stop_profiling()
    aeg_err stop_profiling(int h) {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
            return aeg_err::backend_not_ready;
        }
        return graph->stop_profiling(h);
    }

    AIEGraphSim *getGraphSimImpl() override {
        if (!graph) {
            AEG_ERROR("%s: backend not ready", __func__);
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

PROFILE_ID BasePort::start_profiling(int option, uint32_t value) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (g)
        return g->start_profiling(portName, option, value);
    return -1;
}

long long BasePort::read_profiling(PROFILE_ID h) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (g)
        return g->read_profiling(h);
    return -1;
}

void BasePort::stop_profiling(PROFILE_ID h) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (g)
        g->stop_profiling(h);
}

void InputGMIO::gm2aie_nb(char *data, int len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getGmioApiImpl(portName);
    if (apiImpl)
        apiImpl->gm2aie_nb(data, len);
}

void InputGMIO::gm2aie(char *data, int len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getGmioApiImpl(portName);
    if (apiImpl)
        apiImpl->gm2aie(data, len);
}

void OutputGMIO::aie2gm_nb(char *data, int len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getGmioApiImpl(portName);
    if (apiImpl)
        apiImpl->aie2gm_nb(data, len);
}

void OutputGMIO::aie2gm(char *data, int len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getGmioApiImpl(portName);
    if (apiImpl)
        apiImpl->aie2gm(data, len);
}

void OutputGMIO::gmio_wait(bool syncmode) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getGmioApiImpl(portName);
    if (apiImpl)
        apiImpl->wait(syncmode);
}

#endif // !defined(__AIESIM__)

#if !defined(__AIESIM__)
void ExternalBuffer::setAddress(void *ptr) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->setAddress(ptr);
}

void ExternalBuffer::wait(const std::string &port_name) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->wait(port_name);
}

void ExternalBuffer::gm2aie_nb(const std::string &port_name) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->gm2aie_nb(port_name);
}

void ExternalBuffer::aie2gm_nb(const std::string &port_name) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->aie2gm_nb(port_name);
}

void ExternalBuffer::setupPingPongBuffers(void *const ptr1, void *const ptr2) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->setupPingPongBuffers(ptr1, ptr2);
}

void ExternalBuffer::gm2aie_nb() {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl) {
        std::string port_name = portName + ".out[0]";
        apiImpl->gm2aie_nb(port_name);
    }
}

void ExternalBuffer::aie2gm_nb() {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getExternalBufferApiImpl(portName);
    if (apiImpl) {
        std::string port_name = portName + ".in[0]";
        apiImpl->aie2gm_nb(port_name);
    }
}

void ExternalBufferPort::wait() {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (apiImpl)
        apiImpl->wait(portName);
}

void ExternalBufferPort::gm2aie_nb() {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (apiImpl)
        apiImpl->gm2aie_nb(portName);
}

void ExternalBufferPort::aie2gm_nb() {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g) {
        size_t lastDot = portName.rfind('.');
        if (lastDot != std::string::npos) {
            std::string bufferName = portName.substr(0, lastDot);
            apiImpl = g->getExternalBufferApiImpl(bufferName);
        }
    }
    if (apiImpl)
        apiImpl->aie2gm_nb(portName);
}

void RTP::update(void *data, size_t len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getRtpApiImpl(portName);
    if (apiImpl)
        apiImpl->update(data, len);
}

void RTP::read(char *data, int len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getRtpApiImpl(portName);
    if (apiImpl)
        apiImpl->read(data, len);
}

void SharedBuffer::update(void *data, size_t len) {
    auto *g = static_cast<::AIEGraphSim *>(graphImpl);
    if (!apiImpl && g)
        apiImpl = g->getSharedBufferApiImpl(portName);
    if (apiImpl)
        apiImpl->update(data, len);
}
#endif // !defined(__AIESIM__)

} // namespace detail
} // namespace aeg
