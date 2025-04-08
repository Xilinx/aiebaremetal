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
#ifndef __GMIO_API_IMPL__
#define __GMIO_API_IMPL__
#include <queue>
#include <cstdint>
#include "xaiemem.h"
#include "common_layer/abr_api_config.h"
#include "common_layer/abr_runtime_api.h"

enum AIE_GMIO_DIR{
	AIE2GM = 0,
	GM2AIE,
};

typedef struct _MemMap{
	_MemMap(uint64_t paddr, uint64_t vaddr, uint64_t dlen) {
		sim_phy_addr = paddr;
		host_virtual_addr = vaddr;
		len = dlen;
	};
	uint64_t sim_phy_addr;
	uint64_t host_virtual_addr;
	uint64_t len;
}MemMap;

class GmioApiImpl {
public:
	GmioApiImpl(std::shared_ptr<abr::gmio_api> gapi, 
			        abr::gmio_config gconfig, 
							std::shared_ptr<XAieMem> mem,
							std::string name):gmio_api(gapi), gmioconfig(gconfig), xaiemem(mem),gmioName(name) {
	}
	/*
	void* malloc(uint32_t len){
		return xaiemem.malloc(len);
	}

	void free(void* addr) {
		xaiemem.free(addr);
	}
	*/
	void wait() {
		gmio_api->wait();
#if defined(__AIESIM__)
		while(!aie2gm_buf_queue.empty()) {
			auto var = aie2gm_buf_queue.front();
			aie2gm_buf_queue.pop();
			xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.host_virtual_addr), var.len);
		}
#endif
	}

	abr::err_code gm2aie_nb(char* data, int len) {
		uint64_t phy_data = 0;
		int ret;

		ret = xaiemem->get_dev_addr(data, &phy_data);
		if (ret)
			return abr::err_code::aie_driver_error;
		xaiemem->sync_for_dev(reinterpret_cast<void *>(data), len);
		gmio_sync_nb(GM2AIE, phy_data, len);
	}

	abr::err_code aie2gm_nb(char* data, int len) {
		uint64_t phy_data = 0;
		int ret;

		ret  = xaiemem->get_dev_addr(data, &phy_data);
		if (ret)
			return abr::err_code::aie_driver_error;
#if defined(__AIESIM__)
		aie2gm_buf_queue.emplace(phy_data, reinterpret_cast<uint64_t>(data), len);
#else
		// due the arm8 strong cache coherent feature, before to invalid the ddr memory
		// for reading the data from DDR,we need to clean the cache, if not the next
		// invalid will clean the dirty cache then invalid it, that means, if the ddr
		// is dirty in current time, future we only get a old value
		xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_data), len);
#endif
		return gmio_sync_nb(AIE2GM, phy_data, len);
	}

	abr::err_code aie2gm(char* data, int len) {
		abr::err_code status = aie2gm_nb(data, len);
		if (status == abr::err_code::ok) {
			wait();
		}
		return status;
	}

	abr::err_code gm2aie(char* data, int len) {
		abr::err_code status = gm2aie_nb(data, len);
		if (status == abr::err_code::ok) {
			wait();
		}
		return status;
	}

	abr::err_code gmio_sync_nb(AIE_GMIO_DIR dir,  uint64_t data, int len) {
		switch (dir) {
			case AIE2GM:
				if (gmioconfig.type != 1) {
					std::cout << "gmio_config.type should be 1 for aie2gm" << std::endl;
				}
				break;
			case GM2AIE:
				if (gmioconfig.type != 0) {
					std::cout << "gmio_config.type should be 0 for gm2aie" << std::endl;
				}
				break;
			default:
				std::cout << " the direction is wrong" << std::endl;
				break;
		}
		return gmio_api->enqueueBD(data, len);
	}
private:
	std::queue<MemMap> aie2gm_buf_queue;
	std::shared_ptr<abr::gmio_api> gmio_api;
	abr::gmio_config gmioconfig;
	std::shared_ptr<XAieMem> xaiemem;
	std::string gmioName;
};
#endif
