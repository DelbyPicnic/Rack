#include "app.hpp"


namespace rack {


SVGScrew::SVGScrew() {
	canSquash = true;
	sw = new SVGWidget();
	addChild(sw);
	angle = rand();
}

void SVGScrew::draw(NVGcontext *vg) {
	// No need to save the state because that is done in the parent
	Vec center = sw->box.getCenter();
	nvgTranslate(vg, center.x, center.y);
	nvgRotate(vg, angle);
	nvgTranslate(vg, -center.x, -center.y);
	sw->draw(vg);
}

} // namespace rack
