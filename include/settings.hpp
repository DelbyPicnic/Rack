#pragma once

#include <string>


namespace rack {


void settingsSave(std::string filename);
void settingsLoad(std::string filename);


extern bool skipAutosaveOnLaunch;

extern bool largerHitBoxes;
extern bool lockModules;


} // namespace rack
