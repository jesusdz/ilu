# Tests

Unit tests live in `code/tests/unit_test_ilu.cpp` — the only file under `code/tests/`. They cover `ilu_core.h` (memory alignment/arenas, strings, clock/ticks, threading (atomics, mutexes, semaphores, threads), file utilities, math, and other platform-layer utilities) and use a small set of custom assertion macros rather than an external test framework.

## Building and running

```
# Windows
make unit_test_ilu
.\build\unit_test_ilu.exe

# Linux
make unit_test_ilu
./build/unit_test_ilu
```
