RISC-V Port of libffi
=====================

Compile and run:

```
./configure --host=riscv64-unknown-elf
make
```

Run tests in Spike:

```
make check RUN_TEST_FLAGS="--target_board=riscv-sim --all"
```
