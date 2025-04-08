#  1 Compile the baremetal runtime libary

## 1.1. Pre-requirement

   a. Petalinux env set up
   
      source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
       
   b. aie meta data cpp format
   
       ./dependency/baremetal_metadata.cpp
       
   c. BOOT.BIN (cdo + platform pdi+ aie hw pdi)
    
      ./dependency/testkernel.BIN

## 1.2 update the aie-rt repo
     
     update this repo to lastest main or main-aie

## 1.3 generate the standalone bsp

    a. source ./depsetup.sh
      
 # 2 compile the aie baremeal runtime library (it will be part of sandalone bsp lib after release)
 
```
cd deploydemo
source ./build_baremetal.sh
```

the said compile will generate the libaiebaremetal_api.a file which should get integrated into libxil.a after
official release

# 2. Build app to use libaiebaremetal_api.a to accelerate the algo

## 2.2 Build the app

  a. build the meta object
  
 ```
 source ./buildmeta.sh
 ```
 
  b. build the main/app source file
  
  ```
  vim ./graph.cpp
  #add new logic
  source ./buildmain.sh
  ```
  
  c. build the app elf
  
  ```
  source ./buildelf.sh	
  ```
  
  d. copy to elf and pre-build test boot.bin into network folder
  
  ```
  cp ./main.elf ~/
  cp ./dependency/testkernel.BIN ~/
  ```

## 2.3  test the app ON board farm


_assume we use the BOARD farm test box vek280-4 and target 6 is APU_

**On systest Host console**

```
<host username>:~$ /proj/systest/bin/systest vek280-4
[vek280-4] Systest# power 0 power 1
[vek280-4] Systest# xsdb
xsdb% conn
xsdb% tar 1
xsdb% device program /home/<your username>/testkernel.BIN
```

**On second client console**

```
ssh muscaria18
muscaria18:~%/opt/systest/common/bin/systest-client
[vek280-4] Systest# connect com0
On systest Host console

xsdb% tar 6
xsdb% rst -proc
xsdb% dow -force /home/<your username>/main.elf
xsdb% con

```
 
