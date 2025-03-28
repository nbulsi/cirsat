# A circuit-based SAT solver (CirSAT)
CirSAT is designed for Electronic Design Automation (EDA), based on the isomorphic logic network And Inverter Graph (AIG)

## Requirements
A modern compiler is required to build the libraries. 
Compiled successfully with Clang 6.0.1, Clang 12.0.0, GCC 7.3.0, and GCC 8.2.0. 
More information can be found in the [documentation](https://cirsat.readthedocs.io/en/latest/)

## How to Compile
```bash
git clone --recursive https://gitee.com/hukunmei/CirSAT.git 
cd CirSAT
mkdir build
cd build
cmake ..
make
./cirsat
```