#include "app.hpp"


namespace mirack {


SVGButton::SVGButton() {
	canSquash = true;
	canGrowHitBox = true;

	//XXX: We still inherit from FramebufferWidget to preserve class hierarchy but we don't really want this
	canCache = false;

	sw = new SVGWidget();
	addChild(sw);
}

void SVGButton::setSVGs(std::shared_ptr<SVG> defaultSVG, std::shared_ptr<SVG> activeSVG) {
	sw->setSVG(defaultSVG);
	box.size = sw->box.size;
	this->defaultSVG = defaultSVG;
	this->activeSVG = activeSVG ? activeSVG : defaultSVG;
}

void SVGButton::onDragStart(EventDragStart &e) {
	EventAction eAction;
	onAction(eAction);
	sw->setSVG(activeSVG);
	dirty = true;
}

void SVGButton::onDragEnd(EventDragEnd &e) {
	sw->setSVG(defaultSVG);
	dirty = true;
}


} // namespace mirack
