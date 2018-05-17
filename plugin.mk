ifndef RACK_DIR
$(error RACK_DIR is not defined)
endif

ifndef SLUG
$(error SLUG is not defined)
endif

STRIP ?= strip

FLAGS += -DSLUG=$(SLUG)
FLAGS += -fPIC
FLAGS += -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
         -I$(RACK_DIR)/dep/nanovg/src -I$(RACK_DIR)/dep/nanosvg/src -I$(RACK_DIR)/dep/oui-blendish \
         -I$(RACK_DIR)/dep/osdialog
FLAGS += -Wno-missing-braces


include $(RACK_DIR)/arch.mk

ifeq ($(ARCH), lin)
	LDFLAGS += -shared
	TARGET := plugin.so
endif

ifeq ($(ARCH), mac)
	LDFLAGS += -shared -undefined dynamic_lookup
	TARGET := plugin.dylib
endif

ifeq ($(ARCH), win)
	LDFLAGS += -shared -L$(RACK_DIR) -lRack
	TARGET := plugin.dll
endif


all: $(TARGET)

include $(RACK_DIR)/compile.mk

ifdef DEPS
	DEP_FLAGS += -fPIC
	include $(RACK_DIR)/dep.mk
endif

clean:
	rm -rfv build $(TARGET) dist

DEB_VERSION = $(VERSION)-$(shell date +%s)
DEB_ARCH = $(shell dpkg --print-architecture)
DEB_SLUG ?= $(SLUG)
DEB = $(RACK_DIR)/dist/miRack-plugin-$(DEB_SLUG)_$(DEB_VERSION)_$(DEB_ARCH).deb
deb: $(DEB)

$(DEB): $(TARGET) $(RACK_DIR)/rel_version.txt debian/control-plugin
	rm -rf dist/work
	mkdir -p dist/work

	mkdir -p dist/work/opt/miRack/plugins/$(DEB_SLUG)
	cp -R $(TARGET) $(DISTRIBUTABLES) dist/work/opt/miRack/plugins/$(DEB_SLUG)
	$(STRIP) -S dist/work/opt/miRack/plugins/$(DEB_SLUG)/$(TARGET)

	mkdir -p dist/work/DEBIAN
	cp -r $(RACK_DIR)/debian/control-plugin dist/work/DEBIAN/control
	sed -i s/__SLUG/$(DEB_SLUG)/ dist/work/DEBIAN/control
	sed -i s/__ARCH/$(shell dpkg --print-architecture)/ dist/work/DEBIAN/control
	sed -i s/__RACKVER/$(shell shtool version -d short $(RACK_DIR)/rel_version.txt)/ dist/work/DEBIAN/control
	sed -i s/__VER/$(DEB_VERSION)/ dist/work/DEBIAN/control

	dpkg-deb --build dist/work $(DEB)

dist: all
	rm -rf dist
	mkdir -p dist/$(SLUG)
	# Strip and copy plugin binary
	cp $(TARGET) dist/$(SLUG)/
ifeq ($(ARCH), mac)
	$(STRIP) -S dist/$(SLUG)/$(TARGET)
else
	$(STRIP) -s dist/$(SLUG)/$(TARGET)
endif
	# Copy distributables
	cp -R $(DISTRIBUTABLES) dist/$(SLUG)/
	# Create ZIP package
	cd dist && zip -5 -r $(SLUG)-$(VERSION)-$(ARCH).zip $(SLUG)


.PHONY: clean dist
.DEFAULT_GOAL := all
