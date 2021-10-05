CXXFLAGS=-fno-split-stack
LDFLAGS+=-lcrypto

.PHONY: clean

wrapper: mambo_tracer_wrapper.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o wrapper

clean:
	rm wrapper