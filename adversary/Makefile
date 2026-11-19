CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
BIN      = bin

# need no team code
STANDALONE = $(BIN)/gen_baseline_rebuild $(BIN)/gen_b_updates $(BIN)/gen_b_queries \
             $(BIN)/check_input $(BIN)/demo_increase_only_fails
# need ../include and ../src/structB
PROBES = $(BIN)/find_worst_updates $(BIN)/find_worst_queries
# needs the oracle, Structure A and Structure B
REPLAY = $(BIN)/run_adversarial

all: $(STANDALONE) $(PROBES) $(REPLAY)
standalone: $(STANDALONE)

$(BIN):
	mkdir -p $(BIN)

$(BIN)/%: %.cpp common.hpp reader.hpp baseline_sim.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/find_worst_updates: find_worst_updates.cpp common.hpp target.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@
$(BIN)/find_worst_queries: find_worst_queries.cpp common.hpp target.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@
$(BIN)/run_adversarial: run_adversarial.cpp ../oracle/oracle.cpp common.hpp reader.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< ../oracle/oracle.cpp -o $@

clean:
	rm -rf $(BIN) out

.PHONY: all standalone clean
