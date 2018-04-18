#include "app.hpp"


namespace rack {


SVGSlider::SVGSlider() {
	canSquash = true;

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

void SVGSlider::step() {
	//TODO: //FIXME:
	//if (dirty)
	{
		// Interpolate handle position
		handle->box.pos = Vec(rescale(value, minValue, maxValue, minHandlePos.x, maxHandlePos.x), rescale(value, minValue, maxValue, minHandlePos.y, maxHandlePos.y));
	}
	Widget::step();
}

void SVGSlider::onChange(EventChange &e) {
	//dirty = true;
	if(FramebufferWidget* v = dynamic_cast<FramebufferWidget*>(parent))
		v->dirty = true;
	Knob::onChange(e);
}


} // namespace rack
