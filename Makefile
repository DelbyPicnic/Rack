RACK_DIR ?= .
VERSION = master

FLAGS += \
	-Iinclude \
	-Idep/include -I$(RACK_DIR)/dep/nanovg/src -Idep/nanosvg/src -Idep/osdialog \
	-Idep/oui-blendish -Idep/lib/libzip/include -Idep/tinythread/source

ifdef RELEASE
	FLAGS += -DRELEASE
endif

# Sources and build flags
SOURCES += $(wildcard src/*.cpp src/*/*.cpp)
SOURCES += dep/nanovg/src/nanovg.c dep/tinythread/source/tinythread.cpp

include arch.mk

ifneq (,$(findstring arm,$(CPU)))
	SOURCES += $(wildcard dep/math-neon/*.c)
endif

ifeq ($(ARCH), lin)
	SOURCES += dep/osdialog/osdialog_gtk2.c
	CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
	LDFLAGS += -rdynamic \
		-lpthread -ldl \
		$(shell pkg-config --libs gtk+-2.0) \
		-Ldep/lib -lglfw -ljansson -lspeexdsp -lcurl -lzip -lrtaudio -lrtmidi -lcrypto -lssl

ifneq (,$(findstring arm,$(CPU)))
	LDFLAGS += -lGLESv2 dep/optimized-routines/lib/libmathlib_static.a
else
	LDFLAGS += -lGL
endif

	TARGET := Rack
endif

ifeq ($(ARCH), mac)
	SOURCES += dep/osdialog/osdialog_mac.m
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++ -lpthread -ldl \
		-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo \
		-Ldep/lib -lglfw -ljansson -lspeexdsp -lcurl -lzip -lrtaudio -lrtmidi -lcrypto -lssl
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

PREREQS = cmake autoconf automake pkg-config libtool
ifeq ($(ARCH), lin)
	PREREQS += libgtk2.0-dev libgles2-mesa-dev libasound-dev
ifneq (,$(findstring arm,$(CPU)))
	PREREQS += libmpfr-dev libmpc-dev libgles2-mesa-dev
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


# This target is not intended for public use
dist: all
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
	strip -S $(BUNDLE)/Contents/MacOS/$(TARGET)
	cp icon.icns $(BUNDLE)/Contents/Resources/

	otool -L $(BUNDLE)/Contents/MacOS/$(TARGET)

	cp dep/lib/libglfw.3.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libjansson.4.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libspeexdsp.1.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libcurl.4.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libzip.5.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/librtaudio.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/librtmidi.4.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libcrypto.1.1.dylib $(BUNDLE)/Contents/MacOS/
	cp dep/lib/libssl.1.1.dylib $(BUNDLE)/Contents/MacOS/

	install_name_tool -change lib/libglfw.3.dylib @executable_path/libglfw.3.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libjansson.4.dylib @executable_path/libjansson.4.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libspeexdsp.1.dylib @executable_path/libspeexdsp.1.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libzip.5.dylib @executable_path/libzip.5.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change librtaudio.dylib @executable_path/librtaudio.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/librtmidi.4.dylib @executable_path/librtmidi.4.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(PWD)/dep/lib/libssl.1.1.dylib @executable_path/libssl.1.1.dylib $(BUNDLE)/Contents/MacOS/$(TARGET)

	otool -L $(BUNDLE)/Contents/MacOS/$(TARGET)

	cp plugins/Fundamental/dist/Fundamental-*.zip $(BUNDLE)/Contents/Resources/Fundamental.zip
	cp -R Bridge/au/dist/VCV-Bridge.component dist/
	cp -R Bridge/vst/dist/VCV-Bridge.vst dist/
	# Make DMG image
	cd dist && ln -s /Applications Applications
	cd dist && ln -s /Library/Audio/Plug-Ins/Components Components
	cd dist && ln -s /Library/Audio/Plug-Ins/VST VST
	cd dist && hdiutil create -srcfolder . -volname Rack -ov -format UDZO Rack-$(VERSION)-$(ARCH).dmg
endif
ifeq ($(ARCH), win)
	mkdir -p dist/Rack
	cp -R LICENSE* res dist/Rack/
	cp $(TARGET) dist/Rack/
	strip dist/Rack/$(TARGET)
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
	cp $(TARGET) Rack.sh dist/Rack/
	strip dist/Rack/$(TARGET)
	cp dep/lib/libspeexdsp.so dist/Rack/
	cp dep/lib/libjansson.so.4 dist/Rack/
	cp dep/lib/libglfw.so.3 dist/Rack/
	cp dep/lib/libcurl.so.4 dist/Rack/
	cp dep/lib/libzip.so.5 dist/Rack/
	cp dep/lib/librtaudio.so dist/Rack/
	cp dep/lib/librtmidi.so.4 dist/Rack/
	cp dep/lib/libssl.so.1.1 dist/Rack/
	cp dep/lib/libcrypto.so.1.1 dist/Rack/
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
