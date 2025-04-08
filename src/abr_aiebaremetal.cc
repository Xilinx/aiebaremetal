/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
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
*/
#include <iostream>
#include <unordered_map>
#ifdef DEBUG_RUNTIME
#include "hw_config.h"
#endif
#include <xaiengine.h>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "aiebaremetal.h"
#include "../example/testuc/xaie_test.h"
#include "../example/testaie/xaie_test_all.h"
#include "abr_configure.h"
#include "xaiemem.h"
#include <unistd.h>
#include "log/klogr.h"
#include "event.h"
#ifdef __AIESIM__
//#include "../include/main_rts.h"
#include <dlfcn.h>
#endif

using namespace abr;
#include "common_layer/abr_api_message.h"
#include "common_layer/abrothers/fal_util.h"

//#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
// #ifdef __AIESIM__
// extern "C" {
// int gradf_init(const std::string& work_path);
// bool gradf_load_elf(const std::string& work_path, std::vector<std::string>& elfInfoPath);
// }
// #endif
//void* handle = NULL;
#ifdef __AIESIM__
extern void* ess_Write32_Ptr;
extern void* ess_Read32_Ptr;

extern int g_start_plio;
extern const char _binary_scsim_config_json_start[];
extern const char _binary_scsim_config_json_end[];

auto len = _binary_scsim_config_json_end - \
				_binary_scsim_config_json_start;
std::string json_file(_binary_scsim_config_json_start, len);

const char* getLibPathfromScsimConfig(std::string& json_file) {
	std::istringstream iss(json_file);
	std::ofstream outfile("temp.txt", std::ios::out);
	outfile << iss.str();
	outfile.close();

	boost::property_tree::ptree pt;
	std::string short_lib_path;

	boost::property_tree::read_json(iss, pt);
	for (auto& child : pt.get_child("SimulationConfig.pl_ip_block")) {
		short_lib_path = child.second.get<std::string>("lib_path");
	}
	char *work_path = "./Work/";//getenv("AIE_COMPILE_WORK");
	size_t len = std::strlen(work_path) + std::strlen(short_lib_path.c_str()) + 1;
	char* lib_path = new char[len];
	std::sprintf(lib_path, "%s%s", work_path, short_lib_path.c_str());
	return lib_path;
}

const char* lib_path = getLibPathfromScsimConfig(json_file);

static void open_dl(const char* lib_path) {
	void* handle = dlopen(lib_path, RTLD_LAZY);
	if (!handle) {
		throw std::runtime_error(dlerror());
	}
	// } catch (const std::exception& e) {
	// 	std::cerr << "dlopen failed: "<< e.what() << std::endl;
	// }
	ess_Write32_Ptr = (void*)dlsym(handle, "ess_Write32");
	ess_Read32_Ptr = (void*)dlsym(handle, "ess_Read32");

	printf("ess_Write32_Ptr = %p\n", ess_Write32_Ptr);
	printf("ess_Read32_Ptr = %p\n", ess_Read32_Ptr);
	return;
}

#endif

extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];


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

void aie_driver_test(XAie_DevInst *DevInst) {
		#ifdef __AIESIM__
		//dump_uc_app();
		std::cout << "aie_driver_test pid = " << getpid()<<std::endl;
		XAie_SetIOBackend(DevInst, XAIE_IO_BACKEND_SIM);
		XAie_UpdateNpiAddr(DevInst, 0xF6D10000);
		//XAie_PartitionInitialize(DevInst, nullptr);
		//XAie_ResetPartition(DevInst);
		/*
		XAie_LocType loc = XAie_TileLoc(12, 3);
		const uint32_t col_0_row_3 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
#ifdef USE_AIE2
		const uint32_t core_control_addr =col_0_row_3 + 0x00032000;
		const uint32_t core_status_addr = col_0_row_3 + 0x00032004;
#else
		const uint32_t core_control_addr =col_0_row_3 + 0x00038000;
		const uint32_t core_status_addr = col_0_row_3 + 0x00038004;
#endif
		std::cout << "aie_driver_test 1" << std::endl;
		uint32_t d0 = 0,d1=0, d2=0;
		uint32_t bloop = 1;
		//while(bloop) {sleep(1);};
		XAie_Read32(DevInst,core_status_addr, &d1);
		std::cout << "aie_driver_test 2" << std::endl;
		XAie_Read32(DevInst,core_control_addr, &d0);
		XAie_Write32(&DevInst,core_control_addr, 2);
		std::cout << "aie_driver_test 3" << std::endl;
		XAie_Read32(DevInst,core_status_addr, &d2);
		std::cout << " 1read status d0 = " << d0 << " d1 = " << d1 << " d2 = " << d2 << std::endl;
    */
		test_aie_load_uc_elf(DevInst);
		//test_aie_shim(DevInst);
		//xaie_test_xaiemem(DevInst);
		//xaie_test_memalloc(DevInst);
		// test_aie_kernel_load(DevInst);
		//test_aie_shim(DevInst);
#endif
}
class BaremetalGraphImpl {
public:
	~BaremetalGraphImpl() {
		XAie_Finish(&DevInst);
	}
	BaremetalGraphImpl(std::string GraphName, uint8_t rpu) {
		struct abr::aiecompiler_options aiecompiler_options;
		//fix me
		aiecompiler_options.broadcast_enable_core = true;//false;
		xaiemem = std::make_shared<XAieMem>(&DevInst);
		auto& dconfig = cfg.get_driver_config();
		auto& gconfig = cfg.get_graph_config(GraphName);
		GraphApi = std::make_shared<abr::graph_api>(&gconfig);
		EventApi = std::make_shared<abr::event>(&cfg);
		auto aiebase = dconfig.base_address;
		//TODO: AIE compiler should provide a rpu base_address, after that change the logic, now just hardcode.
		if (rpu) {
			aiebase = RPU_AIE_BASE;
			printf("RPU aiebase = 0x%x\n", (uint32_t)aiebase);
		}
		XAie_SetupConfig(ConfigPtr,
				dconfig.hw_gen,
				aiebase,//dconfig.base_address,
				dconfig.column_shift,
				dconfig.row_shift,
				dconfig.num_columns,
				dconfig.num_rows,
				dconfig.shim_row,
				dconfig.mem_row_start,
				dconfig.mem_num_rows,
				dconfig.aie_tile_row_start,
				dconfig.aie_tile_num_rows);

		printf("dconfig.hw_gen = %d nrows = %d ncol = %d\n", dconfig.hw_gen, dconfig.num_rows, dconfig.num_columns);

		printf("Before setuppartitionconfig() base address :0x%lx\n", DevInst.BaseAddr);
		AieRC RC = XAie_SetupPartitionConfig(&DevInst, (dconfig.base_address + (dconfig.partition_overlay_start_cols[0] << dconfig.column_shift)),
		 dconfig.partition_overlay_start_cols[0],
		 dconfig.partition_num_cols);

		printf("After setuppartitionconfig() base address :0x%lx\n", DevInst.BaseAddr);

		if(RC != XAIE_OK) {
			printf("Partition initialization failed.\n");
			return;
		}
		
		RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
		if(RC != XAIE_OK) {
			printf("Driver initialization failed.\n");
			return;
		}
		fal_util::initialize(&DevInst);
		abr::config_manager::initialize(&DevInst, dconfig.mem_num_rows, aiecompiler_options.broadcast_enable_core);
		GraphApi->configure();
		#ifdef __AIESIM__
		#ifndef __LOCAL_TEST__
		open_dl(lib_path);
		#endif
		#endif
	}
	void selftestexit() {
		aie_driver_test(&DevInst);
		#ifdef __AIESIM__
		#ifndef __LOCAL_TEST__
		test_compare_lib_load(lib_path, PS_SO);
		#endif
		#endif
		exit(0);
	}
	void init() {
//#ifdef __AIESIM__
//		auto str_work = std::getenv("AIE_COMPILE_WORK");
//		std::vector<std::string> elfInfoPath;
//		gradf_load_elf(str_work, elfInfoPath);
//		gradf_init(str_work);
//#endif
		uint8_t loop = 0;
		while(loop) {
			std::cout << "init - pid = " << getpid() <<std::endl;
			sleep(1);
		}
		std::cout<< "init----" <<std::endl;
		cfg.ConfigureGmios(xaiemem);
		cfg.ConfigureExternalBuffers(xaiemem);
		//cp.Load("/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work/ps/c_rts/aie_control_config.json");
		//driver test
	}
	void run(int num) {
		//gdb_point(1);

#ifdef __AIESIM__
		//FIXME, this is a hacking to start gmio, need to figure out with
		//simulator and aiecompiler team to check what is the proper way

		//start the simulator PLIO
		g_start_plio = 1;
#endif

		GraphApi->run(num);
#ifdef DEBUG_RUNTIME
		if (0) {
			/*
			if(XAie_LoadElf(&DevInst, XAie_TileLoc(12, 3), 
						("/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work/aie/12_0/Release/12_0"),
						XAIE_ENABLE)!=XAIE_OK) {
				std::cout << "load elf failed" << std::endl;
			}
			*/
			//XAie_PmSetColumnClk(&DevInst, 0,13, 1);//crash
			XAie_LocType loc = XAie_TileLoc(12, 3);
			const uint32_t col_12_row_3 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
			const uint32_t core_control_addr =col_12_row_3 + 0x00038000;
			const uint32_t core_status_addr = col_12_row_3 + 0x00038004;
			const uint32_t core_gate_addr = col_12_row_3 + 0x0007FF20;//aie2ps
			uint32_t d2 = 0, g0, gate=0;
			//XAie_Write32(&DevInst,core_control_addr, 1);
			//XAie_Read32(&DevInst,core_status_addr, &d2);
/*
    XAie_LocType locs[5] = {XAie_TileLoc(12, 3), XAie_TileLoc(12, 4), XAie_TileLoc(12, 0), XAie_TileLoc(12, 1), XAie_TileLoc(12, 2), };
    XAie_PmRequestTiles(&DevInst, locs, 5);
*/
			/*
			XAie_Read32(&DevInst,core_gate_addr, &g0);
			XAie_Write32(&DevInst,core_gate_addr, 1);
			*/
			XAie_Read32(&DevInst,core_gate_addr, &gate);
			//*/

			std::cout << "enable core core status is " << d2 << " clock is " << gate  << "before gate" << g0<< std::endl;

			//XAie_LockRelease(&DevInst, XAie_TileLoc(12, 3), XAie_LockInit(1, 1), 1);
			//XAie_LockRelease(&DevInst, XAie_TileLoc(12, 3), XAie_LockInit(1, 1), 1);
			/*
			XAie_LockSetValue(&DevInst, XAie_TileLoc(12, 3), XAie_LockInit(0, 10));
			XAie_LockSetValue(&DevInst, XAie_TileLoc(12, 4), XAie_LockInit(1, 10));
			//*/
			int loop = 0;
			while(loop) {
				XAie_Read32(&DevInst,core_status_addr, &d2);
				usleep(1);
			}
		}
#endif
	}

	void aie2gm_nb(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->aie2gm_nb(data, len);
	}
	void aie2gm(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->aie2gm(data, len);
	}

#ifdef DEBUG_RUNTIME
	void gm2aie_check() {
		int channel = 0, col = 12, row = 0;
		XAie_LocType loc = XAie_TileLoc(12, 0);
		const uint32_t col_12_row_0 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
		uint32_t  mm2sc0_status = 0x00009328;
		uint32_t d1;
		int n = 1;

		while(n-- >= 0) {
			XAie_Read32(&DevInst, col_12_row_0 + mm2sc0_status, &d1);
			std::cout << "mm2s status " << d1 << std::endl;
			usleep(10);
		}
   
	}
#endif
	void gm2aie_nb(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->gm2aie_nb(data, len);
		//gm2aie_check();
	}

	void gm2aie(char* gmio_name, char* data, int len) {
		auto gmio_api_impl = cfg.get_gmio_api_impl(gmio_name);
		gmio_api_impl->gm2aie(data, len);
		//gm2aie_check();
	}

	// External Buffer APIs
	void setAddress(char* externalbuffer_name, const void* ptr){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->setAddress(ptr);
	}

	ERR_CODE wait(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->wait(port_name);
	}

	ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->gm2aie_nb(port_name);
	}

	ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name){
		auto externalbuffer_api_impl = cfg.get_externalbuffer_api_impl(externalbuffer_name);
		externalbuffer_api_impl->aie2gm_nb(port_name);
	}

    // End external Buffer APIs

	ERR_CODE update(const std::string& port_name, char* data, int len)
	{
		abr::rtp_config rt_config = cfg.get_rtp_config(port_name);
		return (ERR_CODE)GraphApi->update(&rt_config, data, len);

	}

	ERR_CODE read(const std::string& port_name, char* data, int len)
	{
		abr::rtp_config rt_config = cfg.get_rtp_config(port_name);
		return (ERR_CODE)GraphApi->read(&rt_config, data, len);
	}

	char* dumpcorelog(int row, int col) {
		return rlog(row, col, (uint64_t)&DevInst);
	}

	void* getaiedevinst() {
		return (void*) &DevInst;
	}

	void wait() {
		GraphApi->wait();
		//test
		uint32_t data[128];
		void * phyd = (void*)0x400;
		//xaiemem->sync_to_local_variable(phyd, data, 128);
	}
	ERR_CODE end() {
		std::cout << "graph end() called" << std::endl;
		return (ERR_CODE)GraphApi->end();
	}

	void gmio_wait(std::string gmio_name) {
		auto gmioApiImpl = cfg.get_gmio_api_impl(gmio_name);
		gmioApiImpl->wait();
	}

	void* malloc(uint32_t len){
		return xaiemem->malloc(len);
	}

	void free(void* addr) {
		xaiemem->free(addr);
	}

	int start_profiling(const std::string& port_name, int option, uint32_t value) {
		//gdb_point(1);
		return EventApi->start_profiling(port_name, option, value);
	}
	int start_profiling(const std::string& port_name1,
			const std::string& port_name2,
			int option,
			uint32_t value) {
		return EventApi->start_profiling(port_name1, port_name2, option,value);
	}
	long long read_profiling(int h) {
		return EventApi->read_profiling(h);
	}
	void stop_profiling(int h) {
		return EventApi->stop_profiling(h);
	}
private:
	ConfigureParser cfg;
	XAie_DevInst DevInst={0};
	std::shared_ptr<abr::graph_api> GraphApi;
	std::shared_ptr<abr::event> EventApi;
	///std::queue<TransactionInfo> me2gmSyncBDs;
	std::shared_ptr<XAieMem> xaiemem;
};

void* BaremetalGraph::malloc(size_t len) {
	return graph->malloc(len);
}

void BaremetalGraph::free(void* mem) {
	graph->free(mem);
}

BaremetalGraph::BaremetalGraph(std::string GraphName, uint8_t rpu) {
	graph = std::make_shared<BaremetalGraphImpl>(GraphName, rpu);
}

BaremetalGraph::~BaremetalGraph() {
}

void BaremetalGraph::selftestexit() {
	graph->selftestexit();
}
void BaremetalGraph::init() {
	graph->init();
}

void BaremetalGraph::run(int num) {
	graph->run(num);
}

void BaremetalGraph::aie2gm_nb(char* name, char* data, int len) {
	graph->aie2gm_nb(name, data, len);
}

void BaremetalGraph::gm2aie_nb(char* name, char* data, int len) {
	graph->gm2aie_nb(name, data, len);
}
void BaremetalGraph::aie2gm(char* name, char* data, int len) {
	graph->aie2gm(name, data, len);
}

void BaremetalGraph::gm2aie(char* name, char* data, int len) {
	graph->gm2aie(name, data, len);
}

void BaremetalGraph::wait() {
	graph->wait();
}
ERR_CODE BaremetalGraph::end() {
	return (ERR_CODE)graph->end();
}

void BaremetalGraph::gmio_wait(std::string gmio_name) {
	graph->gmio_wait(gmio_name);
}

ERR_CODE BaremetalGraph::update(const std::string& port_name, char* data, int len)
{
	return (ERR_CODE)graph->update(port_name, data, len);
}

char* BaremetalGraph::dumpcorelog(int row, int col) {
		return graph->dumpcorelog(row, col);
}

void* BaremetalGraph::getaiedevinst() {
	return graph->getaiedevinst();
}

ERR_CODE BaremetalGraph::read(const std::string& port_name, char* data, int len)
{
	return (ERR_CODE)graph->read(port_name, data, len);
}

// External Buffer APIs
void BaremetalGraph::setAddress(char* externalbuffer_name, const void* ptr){
	return graph->setAddress(externalbuffer_name, ptr);
}

ERR_CODE BaremetalGraph::wait(char* externalbuffer_name, std::string port_name){
	return graph->wait(externalbuffer_name, port_name);
}

ERR_CODE BaremetalGraph::gm2aie_nb(char* externalbuffer_name, std::string port_name){
	return graph->gm2aie_nb(externalbuffer_name, port_name);
}

ERR_CODE BaremetalGraph::aie2gm_nb(char* externalbuffer_name, std::string port_name){
	return graph->aie2gm_nb(externalbuffer_name, port_name);
}
// End external Buffer API

int BaremetalGraph::start_profiling(const std::string& port_name, int option, uint32_t value) {
	return graph->start_profiling(port_name, option, value);
}
int BaremetalGraph::start_profiling(const std::string& port_name1,
		const std::string& port_name2,
		int option,
		uint32_t value) {
	return graph->start_profiling(port_name1, port_name2, option,value);
}
long long BaremetalGraph::read_profiling(int h) {
	return graph->read_profiling(h);
}
void BaremetalGraph::stop_profiling(int h) {
	graph->stop_profiling(h);
}
#ifdef __AIESIM__
/*
	 for simulator dll export
*/
class BaremetalGraphSimBaseImpl: public BaremetalGraphSimBase {
	public:
		BaremetalGraphSimBaseImpl(const char* gname, const uint8_t rpu) {
			std::string strGame(gname);
			graph = std::make_shared<BaremetalGraph>(strGame, rpu);
		}
		void selftestexit() override {
			graph->selftestexit();
		}
		void init() override {
			graph->init();
		}
		void run(int num) override {
			graph->run(num);
		}
		void aie2gm_nb(char* name, char* data, int len) override {
			graph->aie2gm_nb(name,data, len);
		}
		void gm2aie_nb(char* name, char* data, int len) override {
			graph->gm2aie_nb(name, data, len);
		}
		void aie2gm(char* name, char* data, int len) override {
			graph->aie2gm(name,data, len);
		}
		void gm2aie(char* name, char* data, int len) override {
			graph->gm2aie(name, data, len);
		}
		void wait() override {
			graph->wait();
		}

		// External Buffer APIs
		void setAddress(char* externalbuffer_name, const void* ptr){
			return graph->setAddress(externalbuffer_name, ptr);
		}

		ERR_CODE wait(char* externalbuffer_name, std::string port_name){
			return graph->wait(externalbuffer_name, port_name);
		}

		ERR_CODE gm2aie_nb(char* externalbuffer_name, std::string port_name){
			return graph->gm2aie_nb(externalbuffer_name, port_name);
		}

		ERR_CODE aie2gm_nb(char* externalbuffer_name, std::string port_name){
			return graph->aie2gm_nb(externalbuffer_name, port_name);
		}
		// End external Buffer API

		ERR_CODE end() {
			return graph->end();
		}
		void gmio_wait(std::string gmio_name) override {
			graph->gmio_wait(gmio_name);
		}
		void* malloc(uint32_t len){
			return graph->malloc(len);
		}
		void free(void* addr) {
			graph->free(addr);
		}

		ERR_CODE update(const std::string& port_name, char* data, int len)
		{
			return graph->update(port_name, data, len);
		}

		ERR_CODE read(const std::string& port_name, char* data, int len)
		{
			return graph->read(port_name, data, len);
		}

		char* dumpcorelog(int row, int col) {
			return graph->dumpcorelog(row, col);
		}

		void* getaiedevinst() {
			//printf("getaiedevinst is not supported in simulator\n");
			return graph->getaiedevinst();
		}
		void gdbpoint() {
			gdb_point(1);
		}
  	int start_profiling(const std::string& port_name, int option, uint32_t value) {
			//gdbpoint();
			return graph->start_profiling(port_name, option, value);
		}
  	int start_profiling(const std::string& port_name1,
													 const std::string& port_name2,
													 int option,
													 uint32_t value) {
			return graph->start_profiling(port_name1, port_name2, option,value);
		}
  	long long read_profiling(int h) {
			return graph->read_profiling(h);
		}
  	void stop_profiling(int h) {
			return graph->stop_profiling(h);
		}
	private:
		std::shared_ptr<BaremetalGraph> graph;
};

extern "C" BaremetalGraphSimBase* createBaremetalGraphInstance(const char* gname,const uint8_t rpu) {
    return new BaremetalGraphSimBaseImpl(gname, rpu);
}
#endif
