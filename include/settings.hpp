#pragma once

#include <string>


#define KNOB_SENSITIVITY 0.0015

namespace rack {


void settingsSave(std::string filename);
void settingsLoad(std::string filename);


extern bool skipAutosaveOnLaunch;

extern bool largerHitBoxes;
extern bool lockModules;
extern float knobSensitivity;

extern std::string lastDialogPath;


} // namespace rack
