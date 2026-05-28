CXX       ?= g++
CXXSTD    ?= -std=c++20
WARNINGS  ?= -Wall -Wextra -Werror -pedantic -Wshadow -Wconversion
OPTIMIZE  ?= -O2
CXXFLAGS  += $(CXXSTD) $(WARNINGS) $(OPTIMIZE) -Isrc
LDFLAGS   +=
LDLIBS    += -lpthread

PREFIX    ?= /usr/local
LOCAL_PREFIX ?= $(HOME)/.local
BINDIR    ?= $(LOCAL_PREFIX)/bin

SRC_DIR   = src
BUILD_DIR = build
TEST_DIR  = tests

SRCS      = $(wildcard $(SRC_DIR)/*.cpp)
OBJS      = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TARGET    = $(BUILD_DIR)/torc

TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_BINS = $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/test_%,$(TEST_SRCS))

# ── Header dependency tracking ──────────────────────────
DEPFLAGS  = -MMD -MP
DEPS      = $(OBJS:.o=.d) $(patsubst $(BUILD_DIR)/test_%,$(BUILD_DIR)/test_%.d,$(notdir $(TEST_BINS)))

.PHONY: all clean check test fmt lint install-local uninstall-local setup

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

# ── Tests ───────────────────────────────────────────────
test: $(TEST_BINS)
	@PASS=0; FAIL=0; \
	for t in $(TEST_BINS); do \
		if $$t; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAIL: $$t"; fi; \
	done; \
	echo "$$PASS passed, $$FAIL failed"; \
	[ $$FAIL -eq 0 ]

$(BUILD_DIR)/test_%: $(TEST_DIR)/%.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) -o $@ $< $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) $(LDLIBS)

# ── Quality ─────────────────────────────────────────────
fmt:
	@find $(SRC_DIR) $(TEST_DIR) -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i

lint:
	@find $(SRC_DIR) -name '*.cpp' | xargs clang-tidy --quiet -p $(BUILD_DIR)

check: fmt lint all test
	@echo "✅ All checks passed"

# ── Install ─────────────────────────────────────────────
install-local: check
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/torc

uninstall-local:
	rm -f $(BINDIR)/torc

# ── Setup ───────────────────────────────────────────────
setup:
	@git config core.hooksPath hooks
	@echo "✅ Pre-commit hook enabled (hooks/pre-commit)"

# ── Clean ───────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
