#include "app.hpp"


namespace rack {


SVGSlider::SVGSlider() {
	canSquash = true;

	//XXX: We still inherit from FramebufferWidget to preserve class hierarchy but we don't really want this
	canCache = false;

	background = new SVGWidget();
	addChild(background);

	handle = new SVGWidget();
	addChild(handle);
}

void SVGSlider::setSVGs(std::shared_ptr<SVG> backgroundSVG, std::shared_ptr<SVG> handleSVG) {
	background->setSVG(backgroundSVG);
	box.size = background->box.size;
	if (handleSVG) {
		handle->setSVG(handleSVG);
	}
}

void SVGSlider::onChange(EventChange &e) {
	handle->box.pos = Vec(rescale(value, minValue, maxValue, minHandlePos.x, maxHandlePos.x), rescale(value, minValue, maxValue, minHandlePos.y, maxHandlePos.y));
	dirty = true;
	Knob::onChange(e);
}


} // namespace rack
