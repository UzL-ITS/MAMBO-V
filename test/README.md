# MAMBO Tests

## Build & Run RISC-V Scanner Test: `test_scanner_riscv`
The scanner test suite is build with [Unity](http://www.throwtheswitch.org/unity). MAMBO's test directory already contains the source files of Unity needed to build and run the tests.

### To build and automatically run `test_scanner_riscv`:

- Build native (on GCC RISC-V 64-bit platform)
	```
	make test_scanner_riscv
	```
	Requirements:
	- **pie** compiled for native RISC-V platform

- Cross-compile for RISC-V target
	```
	CC=riscv64-unknown-linux-gnu-gcc make test_scanner_riscv
	```
	Requirements:
	- RISC-V GNU toolchain (for GCC cross-compiler)
	- **pie** (cross-)compiled for target platform
	
	*NOTE: Expect error 126 at makefile:49 because build platform cannot execute program compiled for RISC-V. But the binary is successfully created. Copy it to a RISC-V device and run your tests there.*

- Build for other platform (e.g. X86-64)
	```
	ARCH=riscv64 make test_scanner_riscv
	```
	Requirements:
	- **pie** compiled for current platform (e.g. X86-64)