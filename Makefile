RACK_DIR ?= .

ifdef RELEASE
	VERSION ?= $(shell shtool version -d short rel_version.txt)
else
	VERSION ?= $(shell shtool version -d short rel_version.txt)-dev
endif

FLAGS += \
	-Iinclude \
	-Idep/include -I$(RACK_DIR)/dep/nanovg/src -Idep/nanosvg/src -Idep/osdialog \
	-Idep/oui-blendish -Idep/lib/libzip/include -Idep/tinythread/source

ifdef RELEASE
	FLAGS += -DRELEASE
endif

include arch.mk

STRIP ?= strip

# Sources and build flags
SOURCES += $(wildcard src/*.cpp src/*/*.cpp)
SOURCES += dep/nanovg/src/nanovg.c dep/tinythread/source/tinythread.cpp

ifneq (,$(findstring arm,$(CPU)))
	SOURCES += $(wildcard dep/math-neon/*.c)
endif

ifeq ($(ARCH), lin)
	SOURCES += dep/osdialog/osdialog_gtk2.c
	CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
	LDFLAGS += -rdynamic \
		-lpthread -ldl -lz -lasound -lX11 \
		$(shell pkg-config --libs gtk+-2.0) \
		-Ldep/lib -Wl,-Bstatic -lglfw3 -ljansson -lspeexdsp -lzip -lz -lrtmidi -lrtaudio -lcurl -lssl -lcrypto -Wl,-Bdynamic

ifneq (,$(findstring arm,$(CPU)))
	LDFLAGS += -lGLESv2 dep/lib/libmathlib_static.a
else
	LDFLAGS += -lGL
endif

	TARGET := Rack
endif

ifeq ($(ARCH), mac)
	SOURCES += dep/osdialog/osdialog_mac.m
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++ \
		-lpthread -ldl -lz \
		-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -framework CoreAudio -framework CoreMIDI \
		dep/lib/libglfw3.a dep/lib/libjansson.a dep/lib/libspeexdsp.a dep/lib/libzip.a dep/lib/librtaudio.a dep/lib/librtmidi.a dep/lib/libcrypto.a dep/lib/libssl.a dep/lib/libcurl.a
	TARGET := Rack
	BUNDLE := dist/$(TARGET).app
endif

ifeq ($(ARCH), win)
	SOURCES += dep/osdialog/osdialog_win.c
	LDFLAGS += -static-libgcc -static-libstdc++ -lpthread -lws2_32 \
		-Wl,--export-all-symbols,--out-implib,libRack.a -mwindows \
		-lgdi32 -lopengl32 -lcomdlg32 -lole32 \
		-Ldep/lib -lglew32 -lglfw3dll -lcurl -lzip -lrtaudio -lrtmidi -lcrypto -lssl \
		-Wl,-Bstatic -ljansson -lspeexdsp
	TARGET := Rack.exe
	OBJECTS += Rack.res
endif


all: $(TARGET)
	@echo ---
	@echo Rack built. Now type \"make run\".
	@echo ---

PREREQS = cmake autoconf automake pkg-config libtool shtool jq
ifeq ($(ARCH), lin)
	PREREQS += libgtk2.0-dev libgles2-mesa-dev libasound2-dev zlib1g-dev
ifneq (,$(findstring arm,$(CPU)))
	PREREQS += libgles2-mesa-dev
else
	PREREQS += libgl1-mesa-dev
endif
	APT = $(shell which apt)
endif
ifeq ($(ARCH), mac)
	PREREQS +=
	BREW = $(shell which brew)
endif

prereq:
ifeq ($(ARCH), lin)
ifeq ($(APT),)
	@echo Install the following packages or their equivalents for your OS: $(PREREQS)
else
	@echo ---
	@echo Will install using apt: $(PREREQS)
	@echo ---
	sudo apt install $(PREREQS)
	@echo ---
	@echo Prerequisites installed. Now type \"make dep\".
	@echo ---
endif
endif

ifeq ($(ARCH), mac)
ifeq ($(BREW),)
	@echo Install Homebrew first from http://brew.sh
else
	@echo ---
	@echo Will install using Homebrew: $(PREREQS)
	@echo ---
	brew install $(PREREQS)
	@echo ---
	@echo Prerequisites installed. Now type \"make dep\".
	@echo ---
endif
endif

dep:
	$(MAKE) -C dep
	@echo ---
	@echo Dependencies built. Now type \"make\".
	@echo ---

run: $(TARGET)
ifeq ($(ARCH), lin)
	LD_LIBRARY_PATH=dep/lib ./$<
endif
ifeq ($(ARCH), mac)
	DYLD_FALLBACK_LIBRARY_PATH=dep/lib ./$<
endif
ifeq ($(ARCH), win)
	env PATH="$(PATH)":dep/bin ./$<
endif

debug: $(TARGET)
ifeq ($(ARCH), lin)
	LD_LIBRARY_PATH=dep/lib gdb -ex run ./Rack
endif
ifeq ($(ARCH), mac)
	DYLD_FALLBACK_LIBRARY_PATH=dep/lib gdb -ex run ./Rack
endif
ifeq ($(ARCH), win)
	env PATH="$(PATH)":dep/bin gdb -ex run ./Rack
endif

perf: $(TARGET)
ifeq ($(ARCH), lin)
	LD_LIBRARY_PATH=dep/lib perf record --call-graph dwarf ./Rack
endif

clean:
	rm -rfv $(TARGET) libRack.a Rack.res build dist

ifeq ($(ARCH), win)
# For Windows resources
%.res: %.rc
	windres $^ -O coff -o $@
endif

DEB_ARCH = $(shell dpkg --print-architecture)
DEB = dist/miRack_$(VERSION)_$(DEB_ARCH).deb
deb: $(DEB)

$(DEB): $(TARGET) $(RACK_DIR)/rel_version.txt debian/control.m4
ifndef RELEASE
	echo Must enable RELEASE for dist target
	exit 1
endif

	rm -rf dist/work
	mkdir -p dist/work

	mkdir -p dist/work/opt/miRack
	cp -R Rack LICENSE* res dist/work/opt/miRack/
	$(STRIP) -S dist/work/opt/miRack/Rack

	mkdir -p dist/work/DEBIAN
	m4 -DARCH=$(DEB_ARCH) -DVER=$(VERSION) debian/control.m4 > dist/work/DEBIAN/control

	fakeroot -- bash -c "chown -R root:root dist/work/* && dpkg-deb --build dist/work $(DEB)"

# This target is not intended for public use
dist: all
ifndef RELEASE
	echo Must enable RELEASE for dist target
	exit 1
endif

	rm -rf dist
	# Rack distribution
	$(MAKE) -C plugins/Fundamental dist

ifeq ($(ARCH), mac)
	mkdir -p $(BUNDLE)
	mkdir -p $(BUNDLE)/Contents
	mkdir -p $(BUNDLE)/Contents/Resources
	cp Info.plist $(BUNDLE)/Contents/
	cp -R LICENSE* res $(BUNDLE)/Contents/Resources

	mkdir -p $(BUNDLE)/Contents/MacOS
	cp $(TARGET) $(BUNDLE)/Contents/MacOS/
	$(STRIP) -S $(BUNDLE)/Contents/MacOS/$(TARGET)
	cp icon.icns $(BUNDLE)/Contents/Resources/

	cp plugins/Fundamental/dist/Fundamental-*.zip $(BUNDLE)/Contents/Resources/Fundamental.zip
	#cp -R Bridge/au/dist/VCV-Bridge.component dist/
	#cp -R Bridge/vst/dist/VCV-Bridge.vst dist/
	# Make DMG image
	cd dist && ln -s /Applications Applications
	cd dist && ln -s /Library/Audio/Plug-Ins/Components Components
	cd dist && ln -s /Library/Audio/Plug-Ins/VST VST
	cd dist && hdiutil create -srcfolder . -volname Rack -ov -format UDZO Rack-$(VERSION)-$(ARCH).dmg
endif
ifeq ($(ARCH), win)
	mkdir -p dist/Rack
	mkdir -p dist/Rack/Bridge
	#cp Bridge/vst/dist/VCV-Bridge-64.dll dist/Rack/Bridge/
	#cp Bridge/vst/dist/VCV-Bridge-32.dll dist/Rack/Bridge/
	cp -R LICENSE* res dist/Rack/
	cp $(TARGET) dist/Rack/
	$(STRIP) -s dist/Rack/$(TARGET)
	cp /mingw64/bin/libwinpthread-1.dll dist/Rack/
	cp /mingw64/bin/zlib1.dll dist/Rack/
	cp /mingw64/bin/libstdc++-6.dll dist/Rack/
	cp /mingw64/bin/libgcc_s_seh-1.dll dist/Rack/
	cp dep/bin/glew32.dll dist/Rack/
	cp dep/bin/glfw3.dll dist/Rack/
	cp dep/bin/libcurl-4.dll dist/Rack/
	cp dep/bin/libjansson-4.dll dist/Rack/
	cp dep/bin/librtmidi-4.dll dist/Rack/
	cp dep/bin/libspeexdsp-1.dll dist/Rack/
	cp dep/bin/libzip-5.dll dist/Rack/
	cp dep/bin/librtaudio.dll dist/Rack/
	cp dep/bin/libcrypto-1_1-x64.dll dist/Rack/
	cp dep/bin/libssl-1_1-x64.dll dist/Rack/
	cp plugins/Fundamental/dist/Fundamental-*.zip dist/Rack/Fundamental.zip
	# Make ZIP
	cd dist && zip -5 -r Rack-$(VERSION)-$(ARCH).zip Rack
	# Make NSIS installer
	makensis installer.nsi
	mv Rack-setup.exe dist/Rack-$(VERSION)-$(ARCH).exe
endif
ifeq ($(ARCH), lin)
	mkdir -p dist/Rack
	cp -R LICENSE* res dist/Rack/
	$(STRIP) -s dist/Rack/$(TARGET)
	cp plugins/Fundamental/dist/Fundamental-*.zip dist/Rack/Fundamental.zip
	# Make ZIP
	cd dist && zip -5 -r Rack-$(VERSION)-$(ARCH).zip Rack
endif

	# Rack SDK distribution
	mkdir -p dist/Rack-SDK
	cp LICENSE* dist/Rack-SDK/
	cp *.mk dist/Rack-SDK/
	cp -R include dist/Rack-SDK/
	mkdir -p dist/Rack-SDK/dep/
	cp -R dep/include dist/Rack-SDK/dep/
ifeq ($(ARCH), win)
	cp libRack.a dist/Rack-SDK/
endif
	cd dist && zip -5 -r Rack-SDK-$(VERSION).zip Rack-SDK


# Plugin helpers

allplugins:
	for f in plugins/*; do $(MAKE) -C "$$f"; done

cleanplugins:
	for f in plugins/*; do $(MAKE) -C "$$f" clean; done

distplugins:
	for f in plugins/*; do $(MAKE) -C "$$f" dist; done

plugins:
	for f in plugins/*; do (cd "$$f" && ${CMD}); done


# Includes

include compile.mk
include plugin-list.mk

.PHONY: all dep run debug clean dist allplugins cleanplugins distplugins plugins list-plugins
.DEFAULT_GOAL := all
