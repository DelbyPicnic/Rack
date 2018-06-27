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

#define ZIP_STATIC
#include <zip.h>
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
