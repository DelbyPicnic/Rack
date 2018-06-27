#include "app.hpp"


namespace mirack {


SVGScrew::SVGScrew() {
	canSquash = true;
	sw = new SVGWidget();
	addChild(sw);
	angle = 0;
}

void SVGScrew::draw(NVGcontext *vg) {
	// No need to save the state because that is done in the parent
	if (angle) {
		Vec center = sw->box.getCenter();
		nvgTranslate(vg, center.x, center.y);
		nvgRotate(vg, angle);
		nvgTranslate(vg, -center.x, -center.y);
	}
	sw->drawCachedOrFresh(vg);
}

} // namespace mirack
