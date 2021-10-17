DBM_DIR = ../..
DBM_PLUGIN_ROOT = plugins/microwalk

CXXFLAGS+=-fno-split-stack
LDFLAGS+=-lcrypto
export PLUGINS+=$(DBM_PLUGIN_ROOT)/tracer.c $(DBM_PLUGIN_ROOT)/trace_writer.cpp $(DBM_PLUGIN_ROOT)/trace_writer_wrapper.cpp

CXX=$(CROSS_PREFIX)g++

.PHONY: wrapper_clean dbm_clean dbm

all: wrapper dbm

dbm: $(DBM_DIR)/dbm
$(DBM_DIR)/dbm:
	$(MAKE) -C $(DBM_DIR) all

dbm_clean:
	$(MAKE) -C $(DBM_DIR) clean

wrapper: mambo_tracer_wrapper.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

clean:
	$(RM) wrapper

cleanall: dbm_clean clean