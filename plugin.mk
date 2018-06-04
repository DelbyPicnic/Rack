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

DISTRIBUTABLES += $(wildcard LICENSE*) res


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

ifeq ($(ARCH), web)
	TARGET := plugin.bc
	FLAGS += -Dinit=init_$(SLUG) -Dplugin=plugin_$(SLUG)
endif


all: $(TARGET)

include $(RACK_DIR)/compile.mk

ifdef DEPS
	DEP_FLAGS += -fPIC
	include $(RACK_DIR)/dep.mk
endif

clean:
	rm -rfv build $(TARGET) dist

ifeq ($(ARCH), lin)
DEB_VERSION = $(VERSION)-$(shell date +%s)
DEB_ARCH = $(shell dpkg --print-architecture)
DEB_SLUG = $(shell echo $(SLUG) | sed s/[^a-zA-Z0-9-]/-/)
DEB = $(RACK_DIR)/dist/miRack-plugin-$(DEB_SLUG)_$(DEB_VERSION)_$(DEB_ARCH).deb
MF = $(RACK_DIR)/catalog/manifests/$(SLUG).json
NAME = $(strip $(shell jq .name $(MF)))
URL = $(strip $(shell jq .pluginUrl//.manualUrl//.sourceUrl $(MF)))
deb: .deb-built 

.deb-built: $(TARGET) $(RACK_DIR)/rel_version.txt $(RACK_DIR)/debian/control-plugin.m4
	rm -rf dist/work
	mkdir -p dist/work

	mkdir -p dist/work/opt/miRack/plugins/$(SLUG)
	cp -R $(TARGET) $(sort $(DISTRIBUTABLES)) dist/work/opt/miRack/plugins/$(SLUG)
	$(STRIP) -S dist/work/opt/miRack/plugins/$(SLUG)/$(TARGET)

	mkdir -p dist/work/DEBIAN
	m4 -DSLUG=$(DEB_SLUG) -DARCH=$(DEB_ARCH) -DRACKVER=$(shell shtool version -d short $(RACK_DIR)/rel_version.txt) -DVER=$(DEB_VERSION) -DDESCR=$(DESCR) -DNAME=$(NAME) -DURL=$(URL) $(RACK_DIR)/debian/control-plugin.m4 > dist/work/DEBIAN/control

	fakeroot -- bash -c "chown -R root:root dist/work/* && dpkg-deb --build dist/work $(DEB)"
	touch .deb-built
endif

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
