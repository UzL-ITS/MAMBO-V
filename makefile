#PLUGINS+=plugins/branch_count.c
#PLUGINS+=plugins/soft_div.c
#PLUGINS+=plugins/tb_count.c
#PLUGINS+=plugins/mtrace.c plugins/mtrace.S
#PLUGINS+=plugins/cachesim/cachesim.c plugins/cachesim/cachesim.S plugins/cachesim/cachesim_model.c
#PLUGINS+=plugins/poc_log_returns.c
#PLUGINS+=plugins/instruction_mix.c
#PLUGINS+=plugins/strace.c
#PLUGINS+=plugins/symbol_example.c
#PLUGINS+=plugins/memcheck/memcheck.S plugins/memcheck/memcheck.c plugins/memcheck/naive_stdlib.c
#PLUGINS+=plugins/microwalk-tracer/tracer.c plugins/microwalk-tracer/trace_writer.cpp plugins/microwalk-tracer/trace_writer_wrapper.cpp

OPTS= -DDBM_LINK_UNCOND_IMM
OPTS+=-DDBM_INLINE_UNCOND_IMM
OPTS+=-DDBM_LINK_COND_IMM
OPTS+=-DDBM_LINK_CBZ
OPTS+=-DDBM_LINK_TBZ
OPTS+=-DDBM_TB_DIRECT #-DFAST_BT
OPTS+=-DLINK_BX_ALT
OPTS+=-DDBM_INLINE_HASH
#OPTS+=-DDBM_TRACES #-DTB_AS_TRACE_HEAD #-DBLXI_AS_TRACE_HEAD
#OPTS+=-DCC_HUGETLB -DMETADATA_HUGETLB

BUILD_DIR=build
OUT=$(or $(OUTPUT_FILE),dbm)

AS=$(CROSS_PREFIX)as
CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++

DEFS+=-D_GNU_SOURCE -DGIT_VERSION=\"$(shell git describe --abbrev=8 --dirty --always)\"

LDFLAGS+=-static -ldl -Wno-stringop-overflow
LIBS=-lelf -lpthread -lz
HEADERS=*.h makefile
INCLUDES=-I/usr/include/libelf -I.
SOURCES= common.c dbm.c traces.c syscalls.c dispatcher.c signals.c util.S
SOURCES+=api/helpers.c api/plugin_support.c api/branch_decoder_support.c api/load_store.c api/internal.c api/hash_table.c
SOURCES+=elf/elf_loader.c elf/symbol_parser.c

ARCH=$(shell $(CC) -dumpmachine | awk -F '-' '{print $$1}')
ifeq ($(findstring arm, $(ARCH)), arm)
	FLAGS += -march=armv7-a -mfpu=neon
	LDFLAGS += -Wl,-Ttext-segment=$(or $(TEXT_SEGMENT),0xa8000000)
	HEADERS += api/emit_arm.h api/emit_thumb.h
	PIE = pie/pie-arm-encoder.o pie/pie-arm-decoder.o pie/pie-arm-field-decoder.o
	PIE += pie/pie-thumb-encoder.o pie/pie-thumb-decoder.o pie/pie-thumb-field-decoder.o
	SOURCES += arch/aarch32/dispatcher_aarch32.S arch/aarch32/dispatcher_aarch32.c
	SOURCES += arch/aarch32/scanner_t32.c arch/aarch32/scanner_a32.c
	SOURCES += api/emit_arm.c api/emit_thumb.c
endif
ifeq ($(ARCH),aarch64)
	HEADERS += api/emit_a64.h
	LDFLAGS += -Wl,-Ttext-segment=$(or $(TEXT_SEGMENT),0x7000000000)
	PIE += pie/pie-a64-field-decoder.o pie/pie-a64-encoder.o pie/pie-a64-decoder.o
	SOURCES += arch/aarch64/dispatcher_aarch64.S arch/aarch64/dispatcher_aarch64.c
	SOURCES += arch/aarch64/scanner_a64.c
	SOURCES += api/emit_a64.c
endif
ifeq ($(ARCH),riscv64)
	FLAGS += -march=rv64gc
	ARCH_OPTS = -DDBM_ARCH_RISCV64
	HEADERS += api/emit_riscv.h
	LDFLAGS += -Wl,-Ttext-segment=$(or $(TEXT_SEGMENT),0x40000000)
	PIE += pie/pie-riscv-field-decoder.o pie/pie-riscv-encoder.o pie/pie-riscv-decoder.o
	SOURCES += arch/riscv/dispatcher_riscv.s arch/riscv/dispatcher_riscv.c
	SOURCES += arch/riscv/scanner_riscv.c
	SOURCES += api/emit_riscv.c
endif

ifdef PLUGINS
	DEFS += -DPLUGINS_NEW
endif

#DEFS += -DDEBUG
DEFS += $(ARCH_OPTS)
FLAGS+=-gdwarf-4

ASFLAGS+=$(FLAGS)
CFLAGS+=$(FLAGS) -O2 -std=gnu99 -MMD -MP
CXXFLAGS+=$(FLAGS) -O2 -MMD -MP

ifeq ($(findstring -DDEBUG, $(DEFS)), -DDEBUG)
	SOURCES += mambo_logger.c
endif

SOURCES += $(PLUGINS)
OBJECTS += $(patsubst %.s,$(BUILD_DIR)/%.s.o,$(filter %.s,$(SOURCES))) \
	$(patsubst %.S,$(BUILD_DIR)/%.S.o,$(filter %.S,$(SOURCES))) \
	$(patsubst %.c,$(BUILD_DIR)/%.c.o,$(filter %.c,$(SOURCES))) \
	$(patsubst %.cpp,$(BUILD_DIR)/%.cpp.o,$(filter %.cpp,$(SOURCES)))
DEPS := $(OBJECTS:.o=.d)

.PHONY: pie clean cleanall print_arch

all: print_arch pie $(OUT)

print_arch:
	$(info MAMBO: detected architecture "$(ARCH)")

pie:
	@$(MAKE) CC=$(CC) --no-print-directory -C pie/ native

# Assembler code
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Assembler code that must be preprocessed
$(BUILD_DIR)/%.S.o: %.S
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(DEFS) $(OPTS) -c $< -o $@ -flto

# C source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(DEFS) $(OPTS) -c $< -o $@ -flto

# C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEFS) $(OPTS) -c $< -o $@ -flto

$(OUT): $(HEADERS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) -o $@ $(OBJECTS) $(PIE) $(LIBS) $(PLUGIN_ARGS)

cachesim:
	PLUGINS="plugins/cachesim/cachesim.c plugins/cachesim/cachesim.S plugins/cachesim/cachesim_model.c" OUTPUT_FILE=mambo_cachesim make

memcheck:
	PLUGINS="plugins/memcheck/memcheck.S plugins/memcheck/memcheck.c plugins/memcheck/naive_stdlib.c" OUTPUT_FILE=mambo_memcheck make

clean:
	$(RM) -r dbm $(BUILD_DIR)

cleanall: clean
	$(MAKE) -C pie/ clean

.PRECIOUS: api/emit_%.c api/emit_%.h
api/emit_%.c: pie/pie-%-encoder.c api/generate_emit_wrapper.rb
	ruby api/generate_emit_wrapper.rb $< > $@

api/emit_%.h: pie/pie-%-encoder.c api/generate_emit_wrapper.rb
	ruby api/generate_emit_wrapper.rb $< header > $@

-include $(DEPS)

MKDIR_P ?= mkdir -p