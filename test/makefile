ifndef ARCH
	ARCH=$(shell $(CC) -dumpmachine | awk -F '-' '{print $$1}')
endif
ifeq ($(findstring arm, $(ARCH)), arm)
	PIE_ENCODER = ../pie/pie-arm-encoder.o ../pie/pie-thumb-encoder.o
	PIE_DECODER = ../pie/pie-arm-decoder.o ../pie/pie-thumb-decoder.o
	CFLAGS+=-march=armv7-a -mfpu=neon
endif
ifeq ($(ARCH),aarch64)
	PIE_ENCODER = ../pie/pie-a64-encoder.o
	PIE_DECODER = ../pie/pie-a64-decoder.o
endif
ifeq ($(ARCH),riscv64)
	PIE_ENCODER = ../pie/pie-riscv-encoder.o
	PIE_DECODER = ../pie/pie-riscv-decoder.o ../pie/pie-riscv-field-decoder.o
	OPTS+=-DDBM_ARCH_RISCV64
endif

CFLAGS+=-std=gnu99
LDFLAGS+=-lelf -lz -lpthread
UNITY_CFLAGS=-Iunity
UNITY_DEFINE=-DUNITY_OUTPUT_COLOR
LDFLAGS_IGNORE_REFERENCE=-Wl,--unresolved-symbols=ignore-in-object-files

.PHONY: clean clean_mocks

portable: mmap_munmap mprotect_exec self_modifying signals load_store

aarch32: portable hw_div

aarch64: portable

hw_div: hw_div.S
	$(CC) -mcpu=cortex-a15 $< $(LDFLAGS) -o $@

self_modifying: $(PIE_ENCODER) self_modifying.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

signals: $(PIE_ENCODER) signals.c signals.S
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

load_store: $(PIE_ENCODER) $(PIE_DECODER) load_store.c load_store.S
	$(CC) -g $(CFLAGS) $^ $(LDFLAGS) -o $@

test_elf_loader: $(PIE_ENCODER) $(PIE_DECODER) ../elf/elf_loader.c test_elf_loader.c
	$(CC) -g $(CFLAGS) $^ $(LDFLAGS) $(OPTS) -DMODULE_ONLY -o $@

test_scanner_riscv: $(PIE_ENCODER) $(PIE_DECODER) test_scanner_riscv.c ../arch/riscv/scanner_riscv.c unity/unity.c
	$(CC) -g $(CFLAGS) $(UNITY_CFLAGS) $(PIE_ENCODER) $(PIE_DECODER) test_scanner_riscv.c unity/unity.c $(LDFLAGS) $(OPTS) $(UNITY_DEFINE) -DMODULE_ONLY -o $@
	./$@

test_dispatcher_riscv: $(PIE_ENCODER) $(PIE_DECODER) test_dispatcher_riscv.c ../arch/riscv/dispatcher_riscv.c ../arch/riscv/scanner_riscv.c unity/unity.c
	$(CC) -g $(CFLAGS) $(UNITY_CFLAGS) $^ $(LDFLAGS) $(OPTS) $(UNITY_DEFINE) -DMODULE_ONLY -o $@
	./$@

test_util: test_util.c ../util.S ../arch/riscv/dispatcher_riscv.s unity/unity.c
	$(CC) -g $(CFLAGS) $(UNITY_CFLAGS) $^ $(LDFLAGS) $(OPTS) $(UNITY_DEFINE) -DMODULE_ONLY -o $@ $(LDFLAGS_IGNORE_REFERENCE)
	./$@

clean:
	rm -f mmap_munmap mprotect_exec self_modifying signals hw_div load_store test_elf_loader test_scanner_riscv test_dispatcher_riscv test_util
