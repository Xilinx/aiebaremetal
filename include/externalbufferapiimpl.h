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

// This file contains implementation of external buffer runtime APIs. 

#include <queue>
#include <map>
#include <cstdint>
#include "xaiemem.h"
#include "common_layer/abr_api_config.h"
#include "common_layer/abr_runtime_api.h"

typedef struct _TransactionInfo {
    public:
        void* virtualAddr;
        uint64_t physicalAddr;
        size_t size;
       
        _TransactionInfo(void* virtual_addr, uint64_t physical_addr, size_t transaction_size)
        {
            virtualAddr = virtual_addr;
            physicalAddr = physical_addr;
            size = transaction_size;
        }
}TransactionInfo;

class ExternalBufferApiImpl
{
private:
    std::string ebufferName;
	abr::external_buffer_config config;
	const void* ddr_address;
    std::shared_ptr<XAieMem> xaiemem;
    std::queue<TransactionInfo> aie2gm_buf_queue;
    std::queue<TransactionInfo> gm2aie_buf_queue;
    abr::err_code  getAddress(uint64_t& phy_address)
    {
        //use physical address for bare metal and AIESIM
		int ret;
        printf("DDR address from getaddress():%x\n", ddr_address);
		ret = xaiemem->get_dev_addr(const_cast<void*>(ddr_address), &phy_address);
		if (ret)
			return abr::err_code::aie_driver_error;
        return abr::err_code::ok;
    }  

public:
	ExternalBufferApiImpl(abr::external_buffer_config ebconfig, std::string name,std::shared_ptr<XAieMem> mem):config(ebconfig),ebufferName(name), xaiemem(mem){}
	    
    void setAddress(const void* ptr)
    {
        ddr_address = ptr;
        printf("ddr address:%x\n", ddr_address);
    }
	abr::err_code wait(std::string port_name) 
    {
        // Dont wait for DMA to be done for the ping-pong buffer cases
        //if (config.shim_port_configs.empty() || config.num_bufs == 2)
        abr::err_code status = abr::err_code::ok;
        if (config.shimPortConfigs.empty())
            return status;
         
        shim_port_config port_config;
        for (auto& t_portConfig : config.shimPortConfigs) {
            if(port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                break;
            }
        }

        status = (abr::err_code)dma_api::waitDMAChannelDone(1, port_config.column, 0/*shim row*/, port_config.S2MMOrMM2S, port_config.channel);
        #if defined(__AIESIM__)
        //0:S2MM, 1:MM2S
        if(port_config.S2MMOrMM2S){
            while(!gm2aie_buf_queue.empty()) {
                auto var = gm2aie_buf_queue.front();
                gm2aie_buf_queue.pop();
                xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size);
            }
        }
        else{
             while(!aie2gm_buf_queue.empty()) {
                auto var = aie2gm_buf_queue.front();
                aie2gm_buf_queue.pop();
                xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size);
             }
        }
        #endif
    }

	abr::err_code gm2aie_nb(std::string port_name)
    {
        abr::err_code status = abr::err_code::ok;
        if (config.shimPortConfigs.empty())
            return status;
       
        shim_port_config port_config;

        for (auto& t_portConfig : config.shimPortConfigs) {
            if(port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                break;
            }
        }
        int start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address ;
        status = getAddress(phy_address);
        printf("physical address:%x\n", phy_address);
        for (const auto& shim_bd_info : port_config.shimBDInfos) {
            auto buf_idx = shim_bd_info.buf_idx;
            
            uint64_t bd_address = phy_address + shim_bd_info.offset * 4; //adf::dma_buffer_descriptor::offset is in 32bit word, but driver address is in bytes
            status = dma_api::updateBDAddress(1, port_config.column, 0, (uint8_t)shim_bd_info.bd_id, bd_address);

            transaction_size += shim_bd_info.transaction_size;
            if (start_bd < 0)
                start_bd = shim_bd_info.bd_id;
        }
        printf("Transaction size:%d\n", transaction_size);
        #if defined(__AIESIM__)
        TransactionInfo transaction((void *)((uint64_t)ddr_address), phy_address, transaction_size);
		gm2aie_buf_queue.emplace(transaction);
        #endif
		
		xaiemem->sync_for_dev(reinterpret_cast<void *>((uint64_t)ddr_address), transaction_size);
    
        status = dma_api::enqueueTask(1, port_config.column, 0, port_config.S2MMOrMM2S, port_config.channel, port_config.taskRepetition, port_config.enable_task_complete_token, static_cast<uint8_t>(start_bd));
        
        return status;
    }

    abr::err_code aie2gm_nb(std::string port_name) 
    {
        abr::err_code status = abr::err_code::ok;
        if (config.shimPortConfigs.empty())
            return status;
       
        shim_port_config port_config;

        for (auto& t_port_config : config.shimPortConfigs) {
            if(port_name == t_port_config.portName) {
                port_config = t_port_config;
                break;
            }
        }
        int start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address ;
        status = getAddress(phy_address);
        printf("physical address:%x\n", phy_address);
        for (const auto& shim_bd_info : port_config.shimBDInfos) {
            auto buf_idx = shim_bd_info.buf_idx;
            
            uint64_t bd_address = phy_address + shim_bd_info.offset * 4; //adf::dma_buffer_descriptor::offset is in 32bit word, but driver address is in bytes
            status = dma_api::updateBDAddress(1, port_config.column, 0/*shim row*/, (uint8_t)shim_bd_info.bd_id, bd_address);
            transaction_size += shim_bd_info.transaction_size;
            if (start_bd < 0)
                start_bd = shim_bd_info.bd_id;
        }
       
        #if defined(__AIESIM__)
        TransactionInfo transaction((void *)((uint64_t)ddr_address), phy_address, transaction_size);
		aie2gm_buf_queue.emplace(transaction);
        #else
		
		xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_address), transaction_size);
        #endif
        status = dma_api::enqueueTask(1, port_config.column, 0, port_config.S2MMOrMM2S, port_config.channel, port_config.taskRepetition, port_config.enable_task_complete_token, static_cast<uint8_t>(start_bd));
        
        return status;
    } 

};


   
	

	
