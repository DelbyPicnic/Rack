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


namespace rack {

static std::string localDir;
static std::string globalDir;
static std::string hiddenDir;


void assetInit() {
	// Global
#if ARCH_MAC && RELEASE
	CFBundleRef bundle = CFBundleGetMainBundle();
	assert(bundle);
	CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(bundle);
	char buf[PATH_MAX];
	Boolean success = CFURLGetFileSystemRepresentation(resourcesUrl, TRUE, (UInt8 *)buf, sizeof(buf));
	assert(success);
	CFRelease(resourcesUrl);
	globalDir = buf;

#else
	globalDir = ".";
#endif


	// Local
#if ARCH_MAC
	// Use home directory
	struct passwd *pw = getpwuid(getuid());
	assert(pw);
	localDir = pw->pw_dir;
	localDir += "/Documents";
	mkdir(localDir.c_str(), 0755);
#endif

#if ARCH_WIN
	// Use My Documents folder
	char buf[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, buf);
	assert(result == S_OK);
	localDir = buf;
	localDir += "/Rack";
	CreateDirectory(localDir.c_str(), NULL);
#endif

#if ARCH_LIN
	const char *home = getenv("HOME");
	if (!home) {
		struct passwd *pw = getpwuid(getuid());
		assert(pw);
		home = pw->pw_dir;
	}
	localDir = home;

	// If the distro already has Documents folder, use it, otherwise just use home folder
	if (systemIsDirectory(localDir+"/Documents"))
		localDir += "/Documents";
#endif

#if ARCH_WEB
	localDir = "/work";
#endif	


	// Hidden
#ifndef ARCH_WEB
#if RELEASE
#if ARCH_MAC
	// Use Application Support folder
	struct passwd *pw = getpwuid(getuid());
	assert(pw);
	hiddenDir = pw->pw_dir;
	hiddenDir += "/Library/Application Support/miRack";
	mkdir(hiddenDir.c_str(), 0755);
#endif
	
#if ARCH_WIN
	// Use Application Data folder
	char buf[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	assert(result == S_OK);
	hiddenDir = buf;
	hiddenDir += "/miRack";
	CreateDirectory(hiddenDir.c_str(), NULL);
#endif

#if ARCH_LIN
	// Use a hidden directory
	const char *home = getenv("HOME");
	if (!home) {
		struct passwd *pw = getpwuid(getuid());
		assert(pw);
		home = pw->pw_dir;
	}
	hiddenDir = home;
	hiddenDir += "/.miRack";
	mkdir(hiddenDir.c_str(), 0755);
#endif

#else
	hiddenDir = ".";
#endif

#else
	hiddenDir = "/work";
#endif
}

// Resources folder if packaged as an app on Mac, current (executable) folder otherwise
std::string assetGlobal(std::string filename) {
	return globalDir + "/" + filename;
}

// User Documents folder always
std::string assetLocal(std::string filename) {
	return localDir + "/" + filename;
}

// Platform-specific folder if RELEASE, current folder otherwise
std::string assetHidden(std::string filename) {
	return hiddenDir + "/" + filename;
}


std::string assetPlugin(Plugin *plugin, std::string filename) {
	assert(plugin);
	return plugin->path + "/" + filename;
}


} // namespace rack
