include $(RACK_DIR)/arch.mk

DEP_LOCAL ?= .
DEP_FLAGS += -g -O3

ifneq (,$(findstring arm,$(CPU)))
#	DEP_FLAGS += -march=armv8-a+crc -mtune=cortex-a53 -mfpu=neon -mfloat-abi=hard -ffast-math -fno-finite-math-only
	DEP_FLAGS += -march=armv7 -mtune=cortex-a17 -mfpu=neon -mfloat-abi=hard -ffast-math -fno-finite-math-only
	DEP_FLAGS += -I$(RACK_DIR)/dep/optimized-routines/build/include -I$(RACK_DIR)/dep/math-neon
else
	DEP_FLAGS += -march=nocona
endif

ifeq ($(ARCH), mac)
	DEP_FLAGS += -mmacosx-version-min=10.7 -stdlib=libc++
	DEP_LDFLAGS += -mmacosx-version-min=10.7 -stdlib=libc++
endif

DEP_CFLAGS += $(DEP_FLAGS)
DEP_CXXFLAGS += $(DEP_FLAGS)

# Commands
WGET := curl -OL
UNTAR := tar xf
UNZIP := unzip
CONFIGURE := ./configure --prefix="$(realpath $(DEP_LOCAL))"
ifeq ($(ARCH), win)
	CMAKE := cmake -G 'MSYS Makefiles' -DCMAKE_INSTALL_PREFIX="$(realpath $(DEP_LOCAL))"
else
	CMAKE := cmake -DCMAKE_INSTALL_PREFIX="$(realpath $(DEP_LOCAL))"
endif

# Export environment for all dependency targets
$(DEPS): export CFLAGS = $(DEP_CFLAGS)
$(DEPS): export CXXFLAGS = $(DEP_CXXFLAGS)
$(DEPS): export LDFLAGS = $(DEP_LDFLAGS)

dep: $(DEPS)

.PHONY: dep
