RISC-V Port of libffi
=====================

Compile and run:

```
./configure --host=riscv64-unknown-elf
make
```

Run tests in Spike:

1. Create boards directoy:
```
mkdir ~/boards
cp misc/riscv-sim.exp ~/boards
```

2. Run tests:
```
make check RUN_TEST_FLAGS="--target_board=riscv-sim --all"
```
