#include "app.hpp"


namespace mirack {


void MomentarySwitch::onDragStart(EventDragStart &e) {
	setValue(maxValue);
	EventAction eAction;
	onAction(eAction);
}

void MomentarySwitch::onDragEnd(EventDragEnd &e) {
	setValue(minValue);
}


} // namespace mirack
