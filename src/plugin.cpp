#include "plugin.hpp"
#include "app.hpp"
#include "asset.hpp"
#include "util/request.hpp"
#include "osdialog.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h> // for MAXPATHLEN
#include <fcntl.h>
#include <thread>
#include <stdexcept>
#include <algorithm>

// #include <zip.h>
#include <jansson.h>

#if ARCH_WIN
	#include <windows.h>
	#include <direct.h>
	#define mkdir(_dir, _perms) _mkdir(_dir)
#else
	#include <dlfcn.h>
#endif
#include <dirent.h>


namespace rack {


std::list<Plugin*> gPlugins;
std::string gToken;


static bool isDownloading = false;
static float downloadProgress = 0.0;
static std::string downloadName;
static std::string loginStatus;


Plugin::~Plugin() {
	for (Model *model : models) {
		delete model;
	}
}

void Plugin::addModel(Model *model) {
	assert(!model->plugin);
	model->plugin = this;
	models.push_back(model);
}

////////////////////
// private API
////////////////////

static bool loadPlugin(std::string path) {
	std::string libraryFilename;
#if ARCH_LIN
	libraryFilename = path + "/" + "plugin.so";
#elif ARCH_WIN
	libraryFilename = path + "/" + "plugin.dll";
#elif ARCH_MAC
	libraryFilename = path + "/" + "plugin.dylib";
#endif

	// Check file existence
	if (!systemIsFile(libraryFilename)) {
		warn("Plugin file %s does not exist", libraryFilename.c_str());
		return false;
	}

	// Load dynamic/shared library
#if ARCH_WIN
	SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	HINSTANCE handle = LoadLibrary(libraryFilename.c_str());
	SetErrorMode(0);
	if (!handle) {
		int error = GetLastError();
		warn("Failed to load library %s: code %d", libraryFilename.c_str(), error);
		return false;
	}
#else
	void *handle = dlopen(libraryFilename.c_str(), RTLD_NOW);
	if (!handle) {
		warn("Failed to load library %s: %s", libraryFilename.c_str(), dlerror());
		return false;
	}
#endif

	// Call plugin's init() function
	typedef void (*InitCallback)(Plugin *);
	InitCallback initCallback;
#if ARCH_WIN
	initCallback = (InitCallback) GetProcAddress(handle, "init");
#else
	initCallback = (InitCallback) dlsym(handle, "init");
#endif
	if (!initCallback) {
		warn("Failed to read init() symbol in %s", libraryFilename.c_str());
		return false;
	}

	// Construct and initialize Plugin instance
	Plugin *plugin = new Plugin();
	plugin->path = path;
	plugin->handle = handle;
	initCallback(plugin);

	// Reject plugin if slug already exists
	Plugin *oldPlugin = pluginGetPlugin(plugin->slug);
	if (oldPlugin) {
		for (Model *model : plugin->models) {
			for (auto it = oldPlugin->models.begin(); it != oldPlugin->models.end();) {
				if (model->slug == (*it)->slug) {
					it = oldPlugin->models.erase(it);
					oldPlugin->models.insert(it, model);
					warn("Plugin %s replaced module \"%s\" of plugin \"%s\"", libraryFilename.c_str(), model->slug.c_str(), plugin->slug.c_str());
				} else
					it++;
			}
		}
	}

	// Add plugin to list
	gPlugins.push_back(plugin);
	info("Loaded plugin %s", libraryFilename.c_str());

	return true;
}

static bool syncPlugin(json_t *pluginJ, bool dryRun) {
/*	json_t *slugJ = json_object_get(pluginJ, "slug");
	if (!slugJ)
		return false;
	std::string slug = json_string_value(slugJ);

	// Get community version
	std::string version;
	json_t *versionJ = json_object_get(pluginJ, "version");
	if (versionJ) {
		version = json_string_value(versionJ);
	}

	// Check whether we already have a plugin with the same slug and version
	for (Plugin *plugin : gPlugins) {
		if (plugin->slug == slug) {
			// plugin->version might be blank, so adding a version of the manifest will update the plugin
			if (plugin->version == version)
				return false;
		}
	}

	json_t *nameJ = json_object_get(pluginJ, "name");
	if (!nameJ)
		return false;
	std::string name = json_string_value(nameJ);

	std::string download;
	std::string sha256;

	json_t *downloadsJ = json_object_get(pluginJ, "downloads");
	if (downloadsJ) {
#if ARCH_WIN
	#define DOWNLOADS_ARCH "win"
#elif ARCH_MAC
	#define DOWNLOADS_ARCH "mac"
#elif ARCH_LIN
	#define DOWNLOADS_ARCH "lin"
#endif
		json_t *archJ = json_object_get(downloadsJ, DOWNLOADS_ARCH);
		if (archJ) {
			// Get download URL
			json_t *downloadJ = json_object_get(archJ, "download");
			if (downloadJ)
				download = json_string_value(downloadJ);
			// Get SHA256 hash
			json_t *sha256J = json_object_get(archJ, "sha256");
			if (sha256J)
				sha256 = json_string_value(sha256J);
		}
	}

	json_t *productIdJ = json_object_get(pluginJ, "productId");
	if (productIdJ) {
		download = gApiHost;
		download += "/download";
		download += "?slug=";
		download += slug;
		download += "&token=";
		download += requestEscape(gToken);
	}

	if (download.empty()) {
		warn("Could not get download URL for plugin %s", slug.c_str());
		return false;
	}

	// If plugin is not loaded, download the zip file to /plugins
	downloadName = name;
	downloadProgress = 0.0;

	// Download zip
	std::string pluginsDir = assetLocal("plugins");
	std::string pluginPath = pluginsDir + "/" + slug;
	std::string zipPath = pluginPath + ".zip";
	bool success = requestDownload(download, zipPath, &downloadProgress);
	if (!success) {
		warn("Plugin %s download was unsuccessful", slug.c_str());
		return false;
	}

	if (!sha256.empty()) {
		// Check SHA256 hash
		std::string actualSha256 = requestSHA256File(zipPath);
		if (actualSha256 != sha256) {
			warn("Plugin %s does not match expected SHA256 checksum", slug.c_str());
			return false;
		}
	}

	downloadName = "";
	return true;*/
}

static void loadPlugins(std::string path) {
	std::string message;
	
	auto entries = systemListEntries(path);
	std::sort (entries.begin(), entries.end());

	for (std::string pluginPath : entries) {
		if (!systemIsDirectory(pluginPath))
			continue;
		if (!loadPlugin(pluginPath)) {
			message += stringf("Could not load plugin %s\n", pluginPath.c_str());
		}
	}
	if (!message.empty()) {
		message += "See log for details.";
		osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
	}
}

/** Returns 0 if successful */
/*static int extractZipHandle(zip_t *za, const char *dir) {
	int err;
	for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
		zip_stat_t zs;
		err = zip_stat_index(za, i, 0, &zs);
		if (err) {
			warn("zip_stat_index() failed: error %d", err);
			return err;
		}
		int nameLen = strlen(zs.name);

		char path[MAXPATHLEN];
		snprintf(path, sizeof(path), "%s/%s", dir, zs.name);

		if (zs.name[nameLen - 1] == '/') {
			if (mkdir(path, 0755)) {
				if (errno != EEXIST) {
					warn("mkdir(%s) failed: error %d", path, errno);
					return errno;
				}
			}
		}
		else {
			zip_file_t *zf = zip_fopen_index(za, i, 0);
			if (!zf) {
				warn("zip_fopen_index() failed");
				return -1;
			}

			FILE *outFile = fopen(path, "wb");
			if (!outFile)
				continue;

			while (1) {
				char buffer[1<<15];
				int len = zip_fread(zf, buffer, sizeof(buffer));
				if (len <= 0)
					break;
				fwrite(buffer, 1, len, outFile);
			}

			err = zip_fclose(zf);
			if (err) {
				warn("zip_fclose() failed: error %d", err);
				return err;
			}
			fclose(outFile);
		}
	}
	return 0;
}*/

/** Returns 0 if successful */
static int extractZip(const char *filename, const char *path) {
/*	int err;
	zip_t *za = zip_open(filename, 0, &err);
	if (!za) {
		warn("Could not open zip %s: error %d", filename, err);
		return err;
	}
	defer({
		zip_close(za);
	});

	err = extractZipHandle(za, path);
	return err;*/
	return 0;
}

static void extractPackages(std::string path) {
/*	std::string message;

	for (std::string packagePath : systemListEntries(path)) {
		if (stringExtension(packagePath) != "zip")
			continue;
		info("Extracting package %s", packagePath.c_str());
		// Extract package
		if (extractZip(packagePath.c_str(), path.c_str())) {
			warn("Package %s failed to extract", packagePath.c_str());
			message += stringf("Could not extract package %s\n", packagePath.c_str());
			continue;
		}
		// Remove package
		if (remove(packagePath.c_str())) {
			warn("Could not delete file %s: error %d", packagePath.c_str(), errno);
		}
	}
	if (!message.empty()) {
		osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
	}*/
}

////////////////////
// public API
////////////////////

std::string pluginPath() {
	std::string path =
#if ARCH_LIN
		assetGlobal("plugins");
#else
		assetHidden("plugins");
#endif

	mkdir(path.c_str(), 0755);
	return path;	
}

#ifdef ARCH_WEB
extern "C" void init_Fundamental(rack::Plugin *plugin);
extern "C" void init_AudibleInstruments(rack::Plugin *plugin);
extern "C" void init_Befaco(rack::Plugin *plugin);
extern "C" void init_cf(rack::Plugin *plugin);
#endif

void pluginInit() {
	tagsInit();

	// Load core
	// This function is defined in core.cpp
	{ Plugin *plugin = new Plugin();
	init(plugin);
	gPlugins.push_back(plugin); }

	{ Plugin *plugin = new Plugin();
	plugin->path = "plugins/Fundamental";
	init_Fundamental(plugin);
	gPlugins.push_back(plugin); }

	{ Plugin *plugin = new Plugin();
	plugin->path = "plugins/AudibleInstruments";
	init_AudibleInstruments(plugin);
	gPlugins.push_back(plugin); }

	{ Plugin *plugin = new Plugin();
	plugin->path = "plugins/Befaco";
	init_Befaco(plugin);
	gPlugins.push_back(plugin); }

	{ Plugin *plugin = new Plugin();
	plugin->path = "plugins/cf";
	init_cf(plugin);
	gPlugins.push_back(plugin); }

	// Get local plugins directory
	std::string localPlugins = pluginPath();

#if RELEASE
	// Copy Fundamental package to plugins directory if folder does not exist
	std::string fundamentalSrc = assetGlobal("Fundamental.zip");
	std::string fundamentalDest = pluginPath() + "/Fundamental.zip";
	std::string fundamentalDir = pluginPath() + "/Fundamental";
	if (!systemIsDirectory(fundamentalDir) && !systemIsFile(fundamentalDest)) {
		systemCopy(fundamentalSrc, fundamentalDest);
	}
#endif

	// Extract packages and load plugins
	extractPackages(localPlugins);
	loadPlugins(localPlugins);
}

void pluginDestroy() {
	for (Plugin *plugin : gPlugins) {
		// Free library handle
#if ARCH_WIN
		if (plugin->handle)
			FreeLibrary((HINSTANCE)plugin->handle);
#else
		if (plugin->handle)
			dlclose(plugin->handle);
#endif

		// For some reason this segfaults.
		// It might be best to let them leak anyway, because "crash on exit" issues would occur with badly-written plugins.
		// delete plugin;
	}
	gPlugins.clear();
}

bool pluginSync(bool dryRun) {
	return false;
	if (gToken.empty())
		return false;

	bool available = false;

	if (!dryRun) {
		isDownloading = true;
		downloadProgress = 0.0;
		downloadName = "Updating plugins...";
	}

	json_t *resJ = NULL;
	json_t *communityResJ = NULL;

	try {
		// Download plugin slugs
		json_t *reqJ = json_object();
		json_object_set(reqJ, "version", json_string(gApplicationVersion.c_str()));
		json_object_set(reqJ, "token", json_string(gToken.c_str()));
		resJ = requestJson(METHOD_GET, gApiHost + "/plugins", reqJ);
		json_decref(reqJ);
		if (!resJ)
			throw std::runtime_error("No response from server");

		json_t *errorJ = json_object_get(resJ, "error");
		if (errorJ)
			throw std::runtime_error(json_string_value(errorJ));

		// Download community plugins
		communityResJ = requestJson(METHOD_GET, gApiHost + "/community/plugins", NULL);
		if (!communityResJ)
			throw std::runtime_error("No response from server");

		json_t *communityErrorJ = json_object_get(communityResJ, "error");
		if (communityErrorJ)
			throw std::runtime_error(json_string_value(communityErrorJ));

		// Check each plugin in list of plugin slugs
		json_t *pluginSlugsJ = json_object_get(resJ, "plugins");
		json_t *communityPluginsJ = json_object_get(communityResJ, "plugins");

		size_t index;
		json_t *pluginSlugJ;
		json_array_foreach(pluginSlugsJ, index, pluginSlugJ) {
			std::string slug = json_string_value(pluginSlugJ);
			// Search for plugin slug in community
			size_t communityIndex;
			json_t *communityPluginJ = NULL;
			json_array_foreach(communityPluginsJ, communityIndex, communityPluginJ) {
				json_t *communitySlugJ = json_object_get(communityPluginJ, "slug");
				if (!communitySlugJ)
					continue;
				std::string communitySlug = json_string_value(communitySlugJ);
				if (slug == communitySlug)
					break;
			}

			// Sync plugin
			if (syncPlugin(communityPluginJ, dryRun)) {
				available = true;
			}
			else {
				warn("Plugin %s not found in community", slug.c_str());
			}
		}
	}
	catch (std::runtime_error &e) {
		warn("Plugin sync error: %s", e.what());
	}

	if (communityResJ)
		json_decref(communityResJ);

	if (resJ)
		json_decref(resJ);

	if (!dryRun) {
		isDownloading = false;
	}

	return available;
}

void pluginLogIn(std::string email, std::string password) {
/*	json_t *reqJ = json_object();
	json_object_set(reqJ, "email", json_string(email.c_str()));
	json_object_set(reqJ, "password", json_string(password.c_str()));
	json_t *resJ = requestJson(METHOD_POST, gApiHost + "/token", reqJ);
	json_decref(reqJ);

	if (resJ) {
		json_t *errorJ = json_object_get(resJ, "error");
		if (errorJ) {
			const char *errorStr = json_string_value(errorJ);
			loginStatus = errorStr;
		}
		else {
			json_t *tokenJ = json_object_get(resJ, "token");
			if (tokenJ) {
				const char *tokenStr = json_string_value(tokenJ);
				gToken = tokenStr;
				loginStatus = "";
			}
		}
		json_decref(resJ);
	}*/
}

void pluginLogOut() {
	gToken = "";
}

void pluginCancelDownload() {
	// TODO
}

bool pluginIsLoggedIn() {
	return gToken != "";
}

bool pluginIsDownloading() {
	return isDownloading;
}

float pluginGetDownloadProgress() {
	return downloadProgress;
}

std::string pluginGetDownloadName() {
	return downloadName;
}

std::string pluginGetLoginStatus() {
	return loginStatus;
}

Plugin *pluginGetPlugin(std::string pluginSlug) {
	for (Plugin *plugin : gPlugins) {
		if (plugin->slug == pluginSlug) {
			return plugin;
		}
	}
	return NULL;
}

Model *pluginGetModel(std::string pluginSlug, std::string modelSlug) {
	Plugin *plugin = pluginGetPlugin(pluginSlug);
	if (plugin) {
		for (Model *model : plugin->models) {
			if (model->slug == modelSlug) {
				return model;
			}
		}
	}
	return NULL;
}


} // namespace rack
