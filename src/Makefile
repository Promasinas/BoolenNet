# Makefile — BoolNet Framework
#
# Modules:
#   mem_int/           Constitution VIII: Integer Memory Layer
#   mem_part/          Float Partition Activation Manager
#   conv1d_circular/   Constitution V: 1D Circular Convolution
#   upsampling/        Constitution VI: Multi-Router Upsampling
#   boolnet/           Constitution IX: Forward Function & Topology
#   tsetlin_train/     Constitution X: Tsetlin Automaton Training

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -I.

.PHONY: all clean mem_int mem_part bool_router conv1d upsampling boolnet tsetlin

all: mem_int mem_part bool_router conv1d upsampling boolnet tsetlin

# ---- mem_int/ ----
mem_int:
	$(CC) $(CFLAGS) -c -o mem_int/mem_int_layer.o mem_int/mem_int_layer.c

# ---- mem_part/ ----
mem_part:
	$(CC) $(CFLAGS) -c -o mem_part/partition.o   mem_part/partition.c
	$(CC) $(CFLAGS) -c -o mem_part/vector_ops.o  mem_part/vector_ops.c
	$(CC) $(CFLAGS) -c -o mem_part/trigger.o     mem_part/trigger.c
	$(CC) $(CFLAGS) -c -o mem_part/part_config.o mem_part/part_config.c
	$(CC) $(CFLAGS) -c -o mem_part/part_mgr.o    mem_part/part_mgr.c

# ---- conv1d_circular/ ----
conv1d:
	$(CC) $(CFLAGS) -c -o conv1d_circular/conv1d_circular.o conv1d_circular/conv1d_circular.c

# ---- upsampling/ ----
upsampling:
	$(CC) $(CFLAGS) -c -o upsampling/upsampling.o upsampling/upsampling.c

# ---- bool_router/ ----
bool_router:
	$(CC) $(CFLAGS) -c -o bool_router/bool_router.o bool_router/bool_router.c

# ---- boolnet/ ----
boolnet:
	$(CC) $(CFLAGS) -c -o boolnet/boolnet.o boolnet/boolnet.c

# ---- tsetlin_train/ ----
tsetlin: boolnet
	$(CC) $(CFLAGS) -c -o tsetlin_train/tsetlin_train.o tsetlin_train/tsetlin_train.c

# ---- LLM Network Integration Test ----
llm_test: all
	$(CC) $(CFLAGS) -I. -o ../test/llm_network_test.exe ../test/llm_network_test.c \
		bool_router/bool_router.o conv1d_circular/conv1d_circular.o \
		upsampling/upsampling.o mem_int/mem_int_layer.o \
		boolnet/boolnet.o tsetlin_train/tsetlin_train.o -lm
	@echo "[OK] llm_network_test.exe built"

# ---- Agent 1 Training Tests ----
train_tests: all
	$(CC) $(CFLAGS) -I. -o ../test/train_test.exe ../test/train_test.c \
		bool_router/bool_router.o mem_int/mem_int_layer.o \
		boolnet/boolnet.o tsetlin_train/tsetlin_train.o -lm
	$(CC) $(CFLAGS) -I. -o ../test/boolnet_train.exe ../test/boolnet_train.c \
		bool_router/bool_router.o mem_int/mem_int_layer.o \
		boolnet/boolnet.o tsetlin_train/tsetlin_train.o -lm
	@echo "[OK] train_test.exe + boolnet_train.exe built"

# ---- Full Train → Save → Load → Infer ----
full_train: all
	$(CC) $(CFLAGS) -I. -o ../test/full_train_save.exe ../test/full_train_save.c \
		bool_router/bool_router.o conv1d_circular/conv1d_circular.o \
		mem_int/mem_int_layer.o boolnet/boolnet.o tsetlin_train/tsetlin_train.o -lm
	@echo "[OK] full_train_save.exe built"

clean:
	rm -f mem_int/*.o mem_part/*.o bool_router/*.o conv1d_circular/*.o upsampling/*.o boolnet/*.o tsetlin_train/*.o ../test/llm_network_test.exe
