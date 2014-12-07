RISC-V Port of libffi
=====================

Compile and run:

```
./configure --host=riscv64-unknown-elf
make
```

Run tests in Spike:

```
make check RUNTESTFLAGS="--target_board=riscv-sim --all"
```

Run an individual test:

Go to `testsuite/libffi.call` directory and run:

```
../../misc/run_test.sh <test_name.c>
```
