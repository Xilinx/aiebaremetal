1. Set up the vitis and vsp environment for build

```
source ./env.sh
```

2. build the UC firmware

```
make
```

3. check the uc firmware/elf by using readelf

```
readelf -S ./output/*.elf
```
