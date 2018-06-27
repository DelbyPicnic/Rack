#include "asset.hpp"
#include "util/common.hpp"
#include <sys/stat.h> // for mkdir
#include "osdialog.h"

#if ARCH_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#endif

#if ARCH_WIN
#include <Windows.h>
#include <Shlobj.h>
#endif

#if ARCH_LIN
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif


namespace mirack {

// Resources folder if packaged as an app on Mac, current (executable) folder otherwise
std::string assetGlobal(std::string filename) {
	std::string dir;

#if ARCH_MAC && RELEASE
	CFBundleRef bundle = CFBundleGetMainBundle();
	assert(bundle);
	CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(bundle);
	char buf[PATH_MAX];
	Boolean success = CFURLGetFileSystemRepresentation(resourcesUrl, TRUE, (UInt8 *)buf, sizeof(buf));
	assert(success);
	CFRelease(resourcesUrl);
	dir = buf;

#else
	dir = ".";
#endif

	return dir + "/" + filename;
}

// User Documents folder always
std::string assetLocal(std::string filename) {
	std::string dir;

#if ARCH_MAC
	// Use home directory
	struct passwd *pw = getpwuid(getuid());
	assert(pw);
	dir = pw->pw_dir;
	dir += "/Documents";
	mkdir(dir.c_str(), 0755);
#endif

#if ARCH_WIN
	// Use My Documents folder
	char buf[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, buf);
	assert(result == S_OK);
	dir = buf;
	dir += "/Rack";
	CreateDirectory(dir.c_str(), NULL);
#endif

#if ARCH_LIN
	const char *home = getenv("HOME");
	if (!home) {
		struct passwd *pw = getpwuid(getuid());
		assert(pw);
		home = pw->pw_dir;
	}
	dir = home;

	// If the distro already has Documents folder, use it, otherwise just use home folder
	if (systemIsDirectory(dir+"/Documents"))
		dir += "/Documents";
#endif

#if ARCH_WEB
	dir = "/work";
#endif

	return dir + "/" + filename;
}

// Platform-specific folder if RELEASE, current folder otherwise
std::string assetHidden(std::string filename) {
	std::string dir;

#ifndef ARCH_WEB
#if RELEASE
#if ARCH_MAC
	// Use Application Support folder
	struct passwd *pw = getpwuid(getuid());
	assert(pw);
	dir = pw->pw_dir;
	dir += "/Library/Application Support/miRack";
	mkdir(dir.c_str(), 0755);
#endif
	
#if ARCH_WIN
	// Use Application Data folder
	char buf[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	assert(result == S_OK);
	dir = buf;
	dir += "/miRack";
	CreateDirectory(dir.c_str(), NULL);
#endif

#if ARCH_LIN
	// Use a hidden directory
	const char *home = getenv("HOME");
	if (!home) {
		struct passwd *pw = getpwuid(getuid());
		assert(pw);
		home = pw->pw_dir;
	}
	dir = home;
	dir += "/.miRack";
	mkdir(dir.c_str(), 0755);
#endif

#else
	dir = ".";
#endif

#else
	dir = "/work";
#endif


	return dir + "/" + filename;
}


std::string assetPlugin(Plugin *plugin, std::string filename) {
	assert(plugin);
	return plugin->path + "/" + filename;
}


} // namespace mirack
