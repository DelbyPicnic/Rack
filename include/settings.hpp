#pragma once

#include <string>


#define KNOB_SENSITIVITY 0.0015

namespace mirack {


void settingsSave(std::string filename);
void settingsLoad(std::string filename);


extern bool skipAutosaveOnLaunch;

extern bool largerHitBoxes;
extern bool lockModules;
extern float knobSensitivity;


} // namespace mirack
