<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# AI Engine (AIE) Applications - Bare-metal

The examples from this Apps folder supports AIE applications for AEG API and has no compilation dependencies on the aiebaremetal/AEG API source code. The build uses the prebuilt AEG API library and headers from Vitis to demonstrate the customer compilation flow.

## Clone the Repo

The repos contains submodules and should be clone recursively:

```bash
$ git clone <aiebaremetal_repo_url> --recursive
```

If the repo is cloned without `--recursive` command, the user can use

```bash
$ git submodule update --init --recursive
```

to download the submodules.

## Repo apps Directory

```
├──plioloopbacktest
```
#  1 Compilling the AIE Baremetal Applications
## 1.1. Pre-requirement

### A. Environment set up
   A.1. Vitis env set up 
    source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
		
		if the said latest build not work, try the following stable build

    source /proj/xbuilds/2025.2_0902_1/installs/lin64/2025.2/Vitis/settings64.sh
### B. Routing the Application
	```bash
	cd aiebaremetal
	cd apps
	```

### C. Source code path
	- plioloopbacktest : source code under `plioloopbacktest/sw/host.cpp`,
	
### D. Compile with aeg appi application [makefile tutorial](./Makefile.md)
	```bash
	$ make build
	```
	
#  2 Testing the AIE Baremetal Applications in board farm
	
#### Target Test board for the said BOOT.BIN and elf

* VEK280 APU & RPU:

*B-revB03,S-prod*
*B-revB03,S-prod*

### vek280 practice on boardfarm

#### 1. Run the fist vek280 console

##### 1.1. Connect into a vek280 board

```
/proj/systest/bin/systest vek280-9
```

##### 1.2. Launch the boot.bin that include the AIE cdos

```
xsdb
connect
dev prog /home/user/vek280/vek280PLIOBOOT.BIN
```

#### 2. Run the second vek280 console

```
ssh @bolete12
/opt/systest/common/bin/systest-client
connect com port as per first console 
```
