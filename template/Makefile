# {{project-name}}
#
# Built on core2 (third-party/core2, a git submodule pinned to a release tag)
# through its core.mk, which compiles the library with this file's CXX and
# CXXFLAGS, so both always agree on every flag (-DNDEBUG included).
#
#   make              debug build: build/{{project-name}}_debug
#   make release      -O3 -march=native -DNDEBUG
#   make lto          release plus full link-time optimization
#   make instrument   AddressSanitizer and UndefinedBehaviorSanitizer
#   make bear         compile_commands.json for clangd
#   make clean        this mode's build; make clean_all removes all of build/

.DEFAULT_GOAL := all

MODE ?= debug

#
# Toolchain and flags
#

CXX := clang++

BASE_FLAGS := -std=c++26 \
              -fno-exceptions \
              -fno-rtti \
              -nostdinc++ \
              -nostdlib++ \
              -Wall \
              -Wextra \
              -Wpedantic \
              -Wglobal-constructors \
              -Wexit-time-destructors \
              -I./src

ifeq ($(MODE),release)
  CXXFLAGS := $(BASE_FLAGS) -O3 -march=native -DNDEBUG
else ifeq ($(MODE),lto)
  CXXFLAGS := $(BASE_FLAGS) -O3 -march=native -DNDEBUG -flto=full
else ifeq ($(MODE),instrument)
  # -fno-sanitize-link-c++-runtime: the sanitizers' C++ runtime needs libc++,
  # which -nostdlib++ leaves out.
  CXXFLAGS := $(BASE_FLAGS) -O1 -g -DBUILD_DEBUG -fno-omit-frame-pointer \
              -fsanitize=address,undefined -fno-sanitize-recover=all \
              -fno-sanitize-link-c++-runtime
else ifeq ($(MODE),debug)
  CXXFLAGS := $(BASE_FLAGS) -O0 -g -DBUILD_DEBUG
else
  $(error unknown MODE=$(MODE); use debug, release, lto or instrument)
endif

#
# Directories and outputs
#

BUILD_DIR   := build/$(MODE)
OBJ_DIR     := $(BUILD_DIR)/obj
COMPDB_DIR  := $(BUILD_DIR)/compdb
TARGET      := build/{{project-name}}_$(MODE)
COMPDB_FILE := compile_commands.json
CORE_DIR    := third-party/core2

RM    := rm -rf
MKDIR := mkdir -p

# clang -MJ fragments for `make bear`
MJFLAGS = $(if $(GENERATE_COMPDB),-MJ $(COMPDB_DIR)/$(subst /,_,$(patsubst $(BUILD_DIR)/%,%,$@)).json)

#
# Modules: core2's and the project's own, through core.mk. CXX and CXXFLAGS
# must be set before the include.
#

ifeq ($(wildcard $(CORE_DIR)/core.mk),)
  $(error $(CORE_DIR) is not checked out: run `git submodule update --init` (in a freshly generated project, `sh setup.sh`))
endif

CORE_BUILD       := $(BUILD_DIR)/core
CORE_EXTRA_FLAGS  = $(MJFLAGS)
include $(CORE_DIR)/core.mk

# The project's modules (src/**/*.cppm) get the same import scan as core2's.
# core_modules stops make on a directory with no modules, hence the guard.
ifneq ($(shell find src -type f -name '*.cppm'),)
  $(call core_modules,APP,src,$(BUILD_DIR)/app)
endif

#
# Sources: every src/**/*.cpp may import core2's modules and the project's
#

CPP_SRCS := $(shell find src -type f -name '*.cpp' | sort)
CPP_OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))
{%- if ui != "plain" %}

#
# UI libraries, through pkg-config. Their flags go on this project's compile
# lines only, not into CXXFLAGS, so core2 is built without them.
#

UI_PACKAGES := {% if ui == "ncurses" %}ncursesw{% elsif ui == "sdl3" %}sdl3{% elsif imgui_backend == "sdl3_renderer" or imgui_backend == "sdl3_gpu" %}sdl3{% elsif imgui_backend == "glfw_opengl3" %}glfw3 gl{% else %}glfw3 vulkan{% endif %}

ifneq ($(shell pkg-config --exists $(UI_PACKAGES) && echo ok),ok)
  $(error pkg-config cannot find $(UI_PACKAGES); install their development files (see README.md))
endif
UI_CFLAGS := $(shell pkg-config --cflags $(UI_PACKAGES))
UI_LIBS   := $(shell pkg-config --libs $(UI_PACKAGES))
{%- endif %}
{%- if ui == "imgui" %}

#
# Dear ImGui (third-party/imgui, a submodule pinned to v1.92.8): the library
# and its {{imgui_backend}} backends. It does not import core2, so it gets its
# own flags: CXXFLAGS without -nostdinc++ and the warnings (it is not this
# project's code), plus -fno-threadsafe-statics, which keeps it off the C++
# runtime's __cxa_guard_*. This project's sources see its headers as system
# headers.
#

IMGUI_DIR   := third-party/imgui
IMGUI_SRCS  := $(addprefix $(IMGUI_DIR)/,imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp \
               imgui_demo.cpp {% if imgui_backend == "sdl3_renderer" %}backends/imgui_impl_sdl3.cpp backends/imgui_impl_sdlrenderer3.cpp{% elsif imgui_backend == "sdl3_gpu" %}backends/imgui_impl_sdl3.cpp backends/imgui_impl_sdlgpu3.cpp{% elsif imgui_backend == "glfw_opengl3" %}backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp{% else %}backends/imgui_impl_glfw.cpp backends/imgui_impl_vulkan.cpp{% endif %})
IMGUI_OBJS  := $(patsubst $(IMGUI_DIR)/%.cpp,$(BUILD_DIR)/imgui/%.o,$(IMGUI_SRCS))
IMGUI_FLAGS := $(filter-out -nostdinc++ -W% -I./src,$(CXXFLAGS)) -fno-threadsafe-statics \
               -I$(IMGUI_DIR) $(UI_CFLAGS)
UI_CFLAGS   += -isystem $(IMGUI_DIR)

ifeq ($(wildcard $(IMGUI_DIR)/imgui.h),)
  $(error $(IMGUI_DIR) is not checked out: run `git submodule update --init` (in a freshly generated project, `sh setup.sh`))
endif
{%- if imgui_backend == "glfw_vulkan" %}

# The loader's vulkan.pc does not bring the headers; they are a package of their own.
ifneq ($(shell printf '\043include <vulkan/vulkan.h>\n' | $(CXX) $(UI_CFLAGS) -fsyntax-only -x c - 2>/dev/null && echo ok),ok)
  $(error vulkan/vulkan.h not found: install the Vulkan headers (Arch: vulkan-headers; Debian, Ubuntu: libvulkan-dev; Fedora: vulkan-headers))
endif
{%- endif %}
{%- endif %}

.PHONY: all debug release lto instrument bear clean clean_all

all: $(TARGET)

debug:
	@$(MAKE) MODE=debug all

release:
	@$(MAKE) MODE=release all

lto:
	@$(MAKE) MODE=lto all

instrument:
	@$(MAKE) MODE=instrument all

# rebuilds with clang -MJ fragments and merges them into compile_commands.json
bear:
	@$(RM) $(COMPDB_FILE)
	@$(RM) $(BUILD_DIR)
	@$(MKDIR) $(COMPDB_DIR)
	@$(MAKE) MODE=$(MODE) GENERATE_COMPDB=1 all
	@{ printf '[\n'; find $(COMPDB_DIR) -type f -name '*.json' | sort | xargs -r cat; } | sed '$$s/,$$//' > $(COMPDB_FILE)
	@printf '\n]\n' >> $(COMPDB_FILE)

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(TARGET)

clean_all:
	$(RM) build
	$(RM) $(COMPDB_FILE)

$(TARGET): $(CPP_OBJS) $(APP_OBJS) $(CORE_OBJS){% if ui == "imgui" %} $(IMGUI_OBJS){% endif %}
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@{% if ui != "plain" %} $(UI_LIBS){% endif %}

# -MMD -MP: header dependencies, for the project's own headers in src/
$(OBJ_DIR)/%.o: src/%.cpp $(CORE_OBJS) $(APP_OBJS) $(CORE_STAMP) $(CORE_HEADERS)
	@$(MKDIR) $(@D) $(if $(GENERATE_COMPDB),$(COMPDB_DIR))
	$(CXX) $(CXXFLAGS){% if ui != "plain" %} $(UI_CFLAGS){% endif %} $(CORE_FLAGS) $(MJFLAGS) -MMD -MP -c $< -o $@
{%- if ui == "imgui" %}

$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp $(CORE_STAMP)
	@$(MKDIR) $(@D) $(if $(GENERATE_COMPDB),$(COMPDB_DIR))
	$(CXX) $(IMGUI_FLAGS) $(MJFLAGS) -c $< -o $@
{%- endif %}

-include $(CPP_OBJS:.o=.d)
