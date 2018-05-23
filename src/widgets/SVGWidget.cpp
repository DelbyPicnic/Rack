#include "widgets.hpp"

namespace rack {


// Some modules set svg directly without calling setSVG() so we need to mark widget dirty
SVGWrapper& SVGWrapper::operator =(std::shared_ptr<SVG> _svg) {
	svg = _svg;
	widget->dirty = true;

	return *this;
};

SVGWrapper& SVGWrapper::operator =(std::nullptr_t _svg) {
	svg = _svg;
	widget->dirty = true;

	return *this;
};

SVGWidget::SVGWidget() : svg(this) {
};

void SVGWidget::wrap() {
	if (svg && svg->image) {
		box.size = Vec(svg->size.x, svg->size.y);
	}
	else {
		box.size = Vec();
	}
}

void SVGWidget::setSVG(std::shared_ptr<SVG> svg) {
	this->svg = svg;
	wrap();
}

void SVGWidget::draw(NVGcontext *vg) {
	if (svg && svg->image) {
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, svg->size.x, svg->size.y);
		nvgTextureQuads(vg, svg->image, 0);
	}
}


} // namespace rack
