# Integer Memory Layer API (Constitution VIII)

**Module**: `src/mem_int/`
**Header**: `src/mem_int/mem_int_layer.h`

## Overview

Constitution VIII compliant integer memory with uint8/16/32/64 precision,
Boolean router trigger recovery, per-forward decay, and binary persistence.

## Quick Start

```c
#include "mem_int/mem_int_layer.h"

MemIntLayer *mem = mem_int_create(2, ML_PRECISION_UINT16, 100, 1000, 5);

uint8_t signal[100] = {0};
signal[0] = 1; signal[42] = 1;
mem_int_forward(mem, signal);   // trigger cells 0,42 → set to max_value

mem_int_save(mem, "memory.bin");
MemIntLayer *loaded = mem_int_load("memory.bin");

mem_int_destroy(mem);
mem_int_destroy(loaded);
```

## Build

```bash
cd src/
make mem_int    # builds mem_int/mem_int_layer.o
```
