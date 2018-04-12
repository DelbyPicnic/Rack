RACK_DIR ?= ..
.NOTPARALLEL:


# Target paths

include $(RACK_DIR)/arch.mk

ifeq ($(ARCH), lin)
	glfw = lib/libglfw.so
	jansson = lib/libjansson.so
	libspeexdsp = lib/libspeexdsp.so
	libcurl = lib/libcurl.so
	libzip = lib/libzip.so
	rtmidi = lib/librtmidi.so
	rtaudio = lib/librtaudio.so
	openssl = lib/libssl.so
	mathlib = optimized-routines/lib/libmathlib_static.a
endif

ifeq ($(ARCH), mac)
	glfw = lib/libglfw.dylib
	jansson = lib/libjansson.dylib
	libspeexdsp = lib/libspeexdsp.dylib
	libcurl = lib/libcurl.dylib
	libzip = lib/libzip.dylib
	rtmidi = lib/librtmidi.dylib
	rtaudio = lib/librtaudio.dylib
	openssl = lib/libssl.dylib
endif

ifeq ($(ARCH), win)
	glew = bin/glew32.dll
	glfw = bin/glfw3.dll
	jansson = bin/libjansson-4.dll
	libspeexdsp = bin/libspeexdsp-1.dll
	libcurl = bin/libcurl-4.dll
	libzip = bin/libzip-5.dll
	rtmidi = bin/librtmidi-4.dll
	rtaudio = bin/librtaudio.dll
	openssl = bin/libssl-1_1-x64.dll
endif

DEPS += $(glew) $(glfw) $(jansson) $(libspeexdsp) $(libcurl) $(libzip) $(rtmidi) $(rtaudio) $(mathlib)
include $(RACK_DIR)/dep.mk


# Targets

$(glew):
	$(WGET) "https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.tgz"
	$(UNTAR) glew-2.1.0.tgz
	cd glew-2.1.0 && mkdir -p build
	cd glew-2.1.0/build && $(CMAKE) -DCMAKE_INSTALL_LIBDIR=lib ./cmake
	$(MAKE) -C glew-2.1.0/build
	$(MAKE) -C glew-2.1.0/build install

$(glfw):
	cd glfw && $(CMAKE) . \
		-DBUILD_SHARED_LIBS=ON \
		-DGLFW_COCOA_CHDIR_RESOURCES=OFF -DGLFW_COCOA_MENUBAR=ON -DGLFW_COCOA_RETINA_FRAMEBUFFER=ON -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
	$(MAKE) -C glfw
	$(MAKE) -C glfw install

$(jansson):
	$(WGET) "http://www.digip.org/jansson/releases/jansson-2.10.tar.gz"
	$(UNTAR) jansson-2.10.tar.gz
	cd jansson-2.10 && $(CONFIGURE)
	$(MAKE) -C jansson-2.10
	$(MAKE) -C jansson-2.10 install

$(libspeexdsp):
	cd speexdsp && ./autogen.sh && $(CONFIGURE)
	$(MAKE) -C speexdsp
	$(MAKE) -C speexdsp install

$(openssl):
	$(WGET) "https://www.openssl.org/source/openssl-1.1.0h.tar.gz"
	$(UNTAR) openssl-1.1.0h.tar.gz
	# ./config ignores CFLAGS, so hack it in with CC
	cd openssl-1.1.0h && CC="$(CC) $(CFLAGS)" ./config --prefix="$(realpath $(DEP_LOCAL))"
	$(MAKE) -C openssl-1.1.0h
	$(MAKE) -C openssl-1.1.0h install_sw
ifeq ($(ARCH),mac)
	otool -L $@
	install_name_tool -change $(PWD)/lib/libcrypto.1.1.dylib @loader_path/libcrypto.1.1.dylib $@
	otool -L $@
endif

$(libcurl): $(openssl)
	$(WGET) "https://curl.haxx.se/download/curl-7.59.0.tar.gz"
	$(UNTAR) curl-7.59.0.tar.gz
	cd curl-7.59.0 && $(CONFIGURE) \
		--disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-manual \
		--without-zlib --without-libpsl --without-libmetalink --without-libssh2 --without-librtmp --without-winidn --without-libidn2 --without-nghttp2 \
		--without-ca-bundle --with-ca-fallback --with-ssl="$(realpath $(DEP_LOCAL))"
	$(MAKE) -C curl-7.59.0
	$(MAKE) -C curl-7.59.0 install
ifeq ($(ARCH),mac)
	otool -L $@
	install_name_tool -change $(PWD)/lib/libcrypto.1.1.dylib @loader_path/libcrypto.1.1.dylib $@
	install_name_tool -change $(PWD)/lib/libssl.1.1.dylib @loader_path/libssl.1.1.dylib $@
	otool -L $@
endif

$(libzip): $(zlib)
	$(WGET) "https://nih.at/libzip/libzip-1.2.0.tar.gz"
	$(UNTAR) libzip-1.2.0.tar.gz
	cd libzip-1.2.0 && $(CONFIGURE)
	$(MAKE) -C libzip-1.2.0
	$(MAKE) -C libzip-1.2.0 install

$(zlib):
	$(WGET) "https://www.zlib.net/zlib-1.2.11.tar.gz"
	$(UNTAR) zlib-1.2.11.tar.gz
ifeq ($(ARCH), win)
	$(MAKE) -C zlib-1.2.11 -f win32/Makefile.gcc
	$(MAKE) -C zlib-1.2.11 -f win32/Makefile.gcc BINARY_PATH="$(realpath $(DEP_LOCAL))/bin" INCLUDE_PATH="$(realpath $(DEP_LOCAL))/include" LIBRARY_PATH="$(realpath $(DEP_LOCAL))/lib" install
else
	cd zlib-1.2.11 && $(CONFIGURE)
	$(MAKE) -C zlib-1.2.11
	$(MAKE) -C zlib-1.2.11 install
endif

$(rtmidi):
	$(WGET) "https://vcvrack.com/downloads/dep/rtmidi.tgz"
	$(UNTAR) rtmidi.tgz
	cd rtmidi && $(CONFIGURE)
	$(MAKE) -C rtmidi
	$(MAKE) -C rtmidi install

$(mathlib):
	cd optimized-routines && $(CMAKE) .
	$(MAKE) -C optimized-routines

ifeq ($(ARCH),mac)
RTAUDIO_FLAGS += -DAUDIO_OSX_CORE=ON
endif
ifeq ($(ARCH),win)
RTAUDIO_FLAGS += -DAUDIO_WINDOWS_DS=ON -DAUDIO_WINDOWS_WASAPI=ON -DAUDIO_WINDOWS_ASIO=ON
endif
ifeq ($(ARCH),lin)
RTAUDIO_FLAGS += -DAUDIO_LINUX_ALSA=ON
endif

ifdef RTAUDIO_ALL_APIS
ifeq ($(ARCH),mac)
RTAUDIO_FLAGS += -DAUDIO_UNIX_JACK=ON
endif
ifeq ($(ARCH),lin)
RTAUDIO_FLAGS += -DAUDIO_LINUX_PULSE=ON -DAUDIO_UNIX_JACK=ON
endif
endif

$(rtaudio):
	cd rtaudio && mkdir -p build
	cd rtaudio/build && $(CMAKE) $(RTAUDIO_FLAGS) ..
	$(MAKE) -C rtaudio/build
	$(MAKE) -C rtaudio/build install

clean:
	git clean -ffdx
