#include "util/common.hpp"
#include "engine.hpp"
#include "window.hpp"
#include "app.hpp"
#include "plugin.hpp"
#include "settings.hpp"
#include "asset.hpp"
#include "bridge.hpp"
#include "osdialog.h"

#include <unistd.h>


using namespace rack;


int main(int argc, char* argv[]) {
	randomInit();
	loggerInit();

	info("Rack %s", gApplicationVersion.c_str());

	{
#if ARCH_LIN
	    char *path = realpath("/proc/self/exe", NULL);
	    if (path) {
	        *(strrchr(path, '/')+1) = 0;
			chdir(path);
			free(path);
	    }
#endif
		char *cwd = getcwd(NULL, 0);
		info("Current working directory: %s", cwd);
		free(cwd);

		info("Global directory: %s", assetGlobal("").c_str());
		info("Local directory: %s", assetLocal("").c_str());
		info("Settings directory: %s", assetHidden("").c_str());
		info("Plugins directory: %s", pluginPath().c_str());		
	}

	pluginInit();
	engineInit();
	// bridgeInit();
	windowInit();
	appInit();
	settingsLoad(assetHidden("settings.json"));
	std::string oldLastPath = gRackWidget->lastPath;

	// To prevent launch crashes, if Rack crashes between now and 15 seconds from now, the "skipAutosaveOnLaunch" property will remain in settings.json, so that in the next launch, the broken autosave will not be loaded.
	bool oldSkipAutosaveOnLaunch = skipAutosaveOnLaunch;
	skipAutosaveOnLaunch = true;
	settingsSave(assetHidden("settings.json"));
	skipAutosaveOnLaunch = false;
	if (oldSkipAutosaveOnLaunch && osdialog_message(OSDIALOG_INFO, OSDIALOG_YES_NO, "Rack has recovered from a crash, possibly caused by a faulty module in your patch. Would you like to clear your patch and start over?")) {
		// Do nothing. Empty patch is already loaded.
	}
	else {
		gRackWidget->loadPatch(assetHidden("autosave.vcv"));
	}
	gRackWidget->lastPath = oldLastPath;

	engineStart();
	windowRun();
	engineStop();

	gRackWidget->savePatch(assetHidden("autosave.vcv"));
	settingsSave(assetHidden("settings.json"));
	appDestroy();
	windowDestroy();
	// bridgeDestroy();
	engineDestroy();
	pluginDestroy();
	loggerDestroy();

	return 0;
}
