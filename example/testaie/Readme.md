# Build and Test

1. git clone --recursive <aiebaremetal repo>

2. cd aiebaremetal

3. make AIE_KERNEL_LOC=$PWD/testaie/kernel AIE_GEN=5 -j32

4. cd ./testaie/kernel

5. source ./runsim.sh

6. the code are in following folder.

   a. testuc
   
   b. testaie
