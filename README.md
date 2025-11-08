# LLVM Assignment 1

The files for the assignment can be found in /lib.
1. **SimpleLICM.cpp**
2. **DerivedInductionVar.cpp**

# Getting Started
make sure you are on the **root** folder for this repo aka **/llvm-tutor**
then run the commands as follows

```
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make
```

## Assignment Files
1. **SimpleLICM** - runs a loop invariant code motion(LICM) pass on a c file 

2. **DerivedInductionVar Pass** - runs a induction variable elimination pass on a c file


## Usage
The following commands show how to run the **LICM** or **Dervied induction variable eliminiation**

**SimpleLICM Pass**
```bash
$LLVM_DIR/bin/opt -load-pass-plugin ./lib/libSimpleLICM.so -passes=simple-licm -S -o [in] [out]
```

**DerivedInductionVar Pass**
```bash
$LLVM_DIR/bin/opt   -load-pass-plugin ./lib/libDerivedInductionVar.so   -passes=derived-iv -S -o [in] [out]
```

### Built in test runs
**SimpleLICM Pass**
```bash
$LLVM_DIR/bin/opt -load-pass-plugin ./lib/libSimpleLICM.so -passes=simple-licm -S -o ../outputs/simple_test.ll ../inputs/simple_test.ll
```

**DerivedInductionVar Pass**
```bash
$LLVM_DIR/bin/opt   -load-pass-plugin ./lib/libDerivedInductionVar.so   -passes=derived-iv -S -o ../outputs/derived_test.ll ../inputs/derived_test.ll
```
### Problems
If you have any problems, follow these steps

in the root folder aka **llvm-tutor** run the commands

```bash
rm -rf build
mkdir build && cd build
```
Then, just simply run the commands from getting started


