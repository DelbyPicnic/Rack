include $(RACK_DIR)/arch.mk

# The install location for `make install`
DEP_LOCAL ?= .
DEP_PATH := $(shell pwd)/$(DEP_LOCAL)

DEP_FLAGS += -g -O3

ifneq (,$(findstring arm,$(CPU)))
	DEP_FLAGS += $(ARM_CPU_FLAGS)
	DEP_FLAGS += -I$(RACK_DIR)/dep/math-neon/src
else
	DEP_FLAGS += -march=nocona
endif

ifeq ($(ARCH), mac)
	DEP_MAC_SDK_FLAGS := -mmacosx-version-min=10.7
	DEP_FLAGS += $(DEP_MAC_SDK_FLAGS) -stdlib=libc++
	DEP_LDFLAGS += $(DEP_MAC_SDK_FLAGS) -stdlib=libc++
endif

DEP_CFLAGS += $(DEP_FLAGS)
DEP_CXXFLAGS += $(DEP_FLAGS)

# Commands
WGET := curl -OL
UNTAR := tar xf
UNZIP := unzip
ifeq ($(ARCH), web)
	CONFIGURE := emconfigure ./configure --prefix="$(DEP_PATH)"
else
	CONFIGURE := ./configure --prefix="$(DEP_PATH)"
endif
ifeq ($(ARCH), win)
	CMAKE := cmake -G 'MSYS Makefiles' -DCMAKE_INSTALL_PREFIX="$(DEP_PATH)"
else
	CMAKE := cmake -DCMAKE_INSTALL_PREFIX="$(DEP_PATH)"
endif

# Export environment for all dependency targets
$(DEPS): export CFLAGS = $(DEP_CFLAGS)
$(DEPS): export CXXFLAGS = $(DEP_CXXFLAGS)
$(DEPS): export LDFLAGS = $(DEP_LDFLAGS)

dep: $(DEPS)

.PHONY: dep
