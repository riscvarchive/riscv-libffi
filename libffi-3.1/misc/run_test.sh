#/bin/bash
riscv64-unknown-elf-gcc -I../../riscv64-unknown-elf/include -I../../riscv64-unknown-elf -L../../riscv64-unknown-elf/.libs/ $1 -lffi 
spike pk a.out
