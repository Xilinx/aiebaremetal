// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

 #include "cert_host.h"

 namespace cert_host {

 cert_api::cert_api(XAie_DevInst *DevInst_in)
     : DevInst(DevInst_in), handshake{}, host_args_low(0), host_args_high(0),
       host_args_meminst(nullptr), hsa_meminst(nullptr), comp_sig_meminst(nullptr) {}

 cert_api::cert_api()
     : DevInst(nullptr), handshake{}, host_args_low(0), host_args_high(0),
       host_args_meminst(nullptr), hsa_meminst(nullptr), comp_sig_meminst(nullptr) {}

 cert_api::~cert_api() {}

 void cert_api::init_devinst(XAie_DevInst *dev_inst) { this->DevInst = dev_inst; }

 int32_t cert_api::load_cert() const { return 0; }

 int32_t cert_api::load_cert(uint32_t) const { return 0; }

 void cert_api::run_cert(uint32_t) const {}

 int32_t cert_api::wakeup_cert() const { return 0; }

 int32_t cert_api::wakeup_cert(uint32_t) const { return 0; }

 int32_t cert_api::init_handshake() { return 0; }

 int32_t cert_api::submit_command(const char *, uint32_t,
                                const std::map<std::string_view, uint64_t> &) const {
     return 0;
 }

 int32_t cert_api::submit_test_command() const { return 0; }

 void cert_api::parse_host_args() {}

 }
