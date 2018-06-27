#include "app.hpp"
#include "window.hpp"
#include "engine.hpp"
#include "settings.hpp"
// For GLFW_KEY_LEFT_CONTROL, etc.
#include <GLFW/glfw3.h>


namespace mirack {


Knob::Knob() {
	smooth = true;
	canGrowHitBox = true;
}

void Knob::onDragStart(EventDragStart &e) {
	windowCursorLock();
	dragValue = value;
	randomizable = false;
}

void Knob::onDragMove(EventDragMove &e) {
	float range;
	if (isfinite(minValue) && isfinite(maxValue)) {
		range = maxValue - minValue;
	}
	else {
		range = 1.0 - (-1.0);
	}
	float delta = knobSensitivity * -e.mouseRel.y * speed * range;

	// Drag slower if Mod is held
	if (windowIsModPressed())
		delta /= 16.0;
	dragValue += delta;
	dragValue = clamp2(dragValue, minValue, maxValue);
	if (snap)
		setValue(roundf(dragValue));
	else
		setValue(dragValue);
}

void Knob::onDragEnd(EventDragEnd &e) {
	windowCursorUnlock();
	randomizable = true;
}


} // namespace mirack
