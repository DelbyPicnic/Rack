#include "app.hpp"


namespace rack {


SVGKnob::SVGKnob() {
	canSquash = true;

	//XXX: We still inherit from FramebufferWidget to preserve class hierarchy but we don't really want this
	canCache = false;

	shadow = new CircularShadow();
	addChild(shadow);
	shadow->box.size = Vec();

	tw = new TransformWidget();
	addChild(tw);

	sw = new SVGWidget();
	tw->addChild(sw);
}

void SVGKnob::setSVG(std::shared_ptr<SVG> svg) {
	sw->setSVG(svg);
	tw->box.size = sw->box.size;
	box.size = sw->box.size;
	shadow->box.size = sw->box.size;
	shadow->box.pos = Vec(0, sw->box.size.y * 0.1);
	// shadow->box = shadow->box.grow(Vec(2, 2));
}

void SVGKnob::onChange(EventChange &e) {
	float angle;
	if (isfinite(minValue) && isfinite(maxValue)) {
		angle = rescale(value, minValue, maxValue, minAngle, maxAngle);
	}
	else {
		angle = rescale(value, -1.0, 1.0, minAngle, maxAngle);
		angle = fmodf(angle, 2*M_PI);
	}
	tw->identity();
	// Rotate SVG
	Vec center = sw->box.getCenter();
	tw->translate(center);
	tw->rotate(angle);
	tw->translate(center.neg());
	dirty = true;
	Knob::onChange(e);
}


} // namespace rack
