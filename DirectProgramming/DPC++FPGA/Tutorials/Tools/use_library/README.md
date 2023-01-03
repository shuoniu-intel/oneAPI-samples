## Create build directory
```
mkdir build
```

## Build SYCL source library
```
fpga_crossgen src/lib_sycl.cpp --source sycl --target sycl -o build/lib_sycl.o
fpga_libtool build/lib_sycl.o --target sycl --create build/lib_sycl.a
```

## Build RTL source library
```
fpga_crossgen src/lib_rtl_spec.xml  --emulation_model src/lib_rtl_model.cpp --target sycl -o build/lib_rtl.o
fpga_libtool build/lib_rtl.o --target sycl --create build/lib_rtl.a
```

## Compile report
```
icpx -fsycl --gcc-toolchain=/p/psg/ctools/gcc/7.4.0/2/linux64/rhel7 -fintelfpga -Xshardware -Xsboard=intel_a10gx_pac:pac_a10 -fsycl-link=early src/use_library.cpp build/lib_rtl.a build/lib_sycl.a -o build/use_library.report
```
