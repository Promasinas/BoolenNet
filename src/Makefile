# Makefile — BoolNet Framework
#
# Structure:
#   common/      Shared types
#   mem_int/     Constitution VIII: Integer Memory Layer
#   mem_part/    Float Partition Activation Manager

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic
INC     = -Icommon -Imem_int -Imem_part

# ---- Integer Memory Layer (constitution VIII) ----
MEM_INT_OBJ = mem_int/mem_int_layer.o

# ---- Partition Activation Manager ----
MEM_PART_OBJ = mem_part/partition.o mem_part/vector_ops.o mem_part/trigger.o \
               mem_part/part_config.o mem_part/part_mgr.o

.PHONY: all clean mem_int mem_part diag

all: mem_int mem_part

# ---- mem_int/ ----
mem_int: $(MEM_INT_OBJ)
	@echo "[OK] Integer Memory Layer built"

mem_int/mem_int_layer.o: mem_int/mem_int_layer.c mem_int/mem_int_layer.h common/boolnet_common.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_int/mem_int_layer.c

# ---- mem_part/ ----
mem_part: $(MEM_PART_OBJ)
	@echo "[OK] Partition Activation Manager built"

mem_part/partition.o: mem_part/partition.c mem_part/partition.h mem_part/part_types.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_part/partition.c

mem_part/vector_ops.o: mem_part/vector_ops.c mem_part/vector_ops.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_part/vector_ops.c

mem_part/trigger.o: mem_part/trigger.c mem_part/trigger.h mem_part/part_types.h mem_part/partition.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_part/trigger.c

mem_part/part_config.o: mem_part/part_config.c mem_part/part_config.h mem_part/partition.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_part/part_config.c

mem_part/part_mgr.o: mem_part/part_mgr.c mem_part/part_mgr.h mem_part/partition.h mem_part/vector_ops.h mem_part/trigger.h
	$(CC) $(CFLAGS) -I. -c -o $@ mem_part/part_mgr.c

# ---- Diagnostic ----
diag: mem_part
	$(CC) $(CFLAGS) -I. -o part_diag.exe mem_part/part_diag.c $(MEM_PART_OBJ)
	@echo "[OK] part_diag.exe built"

clean:
	rm -f mem_int/*.o mem_part/*.o part_diag.exe
