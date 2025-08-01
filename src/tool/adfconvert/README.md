# ADF to Baremetal Runtime Conversion Tool 

## Instructions to test adftoaeg

1. Clone the git repo to a location of your choosing

```bash
git clone https://github.com/Xilinx/aiebaremetal.git
cd aiebaremetal/src/adfconvert
```

2. build

```bash
# first install llvm, then continue

mkdir build
cd build
cmake ../
make
cd ..
```

3. Source the setup script

```bash
source setup.sh
```

4. Run the run.sh script to do the conversion

```bash
./convert.sh  </path/to/target> -out <newfile>
```

For example, run the following command:

```bash
./convert.sh ./example/graph.cpp -out ./graph_new.cpp
```

The input file, `graph.cpp`, is the original code using the adf API. The resulting output file, `graph_new.cpp`, is the code converted to the new aiebaremetal aeg API.

Additionally, if the code you are converting requires extra include paths, you can add them on the command line with the `-I` option.

```bash
./convert.sh </path/to/target> -I/path/to/include/dir1 -I/path/to/include/dir2 -out <newfile>
```
