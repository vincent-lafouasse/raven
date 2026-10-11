# Convenience wrapper around CMake + Ninja.
# Two build trees: build/debug (dev) and build/release.
#
#   make                  build debug
#   make release          build release
#   make TARGET=raven-dis build a single ninja target
#   make run ARGS="foo.bin"
#   make test             run ctest on the debug tree
#   make clean            ninja clean in both trees
#   make fclean           delete build/
#   make re               fclean, then build debug

CMAKE      ?= cmake
NINJA      ?= ninja

# extra arguments passed to every cmake configure
CMAKE_ARGS ?=

# restrict a build to one ninja target (empty means all)
TARGET     ?=

# arguments for `make run`
ARGS       ?=

DEBUG_DIR     := build/debug
RELEASE_DIR   := build/release
DEBUG_NINJA   := $(DEBUG_DIR)/build.ninja
RELEASE_NINJA := $(RELEASE_DIR)/build.ninja

.DEFAULT_GOAL := debug
.PHONY: all build debug release test run clean fclean re

all: build

build: debug release

debug: $(DEBUG_NINJA)
	$(NINJA) -C $(DEBUG_DIR) $(TARGET)
	@ln -sf $(DEBUG_DIR)/compile_commands.json compile_commands.json

release: $(RELEASE_NINJA)
	$(NINJA) -C $(RELEASE_DIR) $(TARGET)

$(DEBUG_NINJA): CMakeLists.txt
	$(CMAKE) -S . -B $(DEBUG_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		$(CMAKE_ARGS)

$(RELEASE_NINJA): CMakeLists.txt
	$(CMAKE) -S . -B $(RELEASE_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		$(CMAKE_ARGS)

test: debug
	cd $(DEBUG_DIR) && ctest --output-on-failure

run: debug
	$(DEBUG_DIR)/bin/raven $(ARGS)

format:
	clang-format -i $(shell find src -name '*.c' -or -name '*.h')
	clang-format -i $(shell find tools -name '*.c' -or -name '*.h')

clean:
	@for d in $(DEBUG_DIR) $(RELEASE_DIR); do \
		if [ -f $$d/build.ninja ]; then $(NINJA) -C $$d clean; fi; \
	done

fclean:
	rm -rf build

re: fclean
	$(MAKE) debug

.PHONY: b t r f
b: build
t: test
r: run
f: format
