#include "app.hpp"
#include "engine.hpp"
#include "componentlibrary.hpp"
#include "window.hpp"


namespace rack {

WireWidget::WireWidget() {
}

WireWidget::~WireWidget() {
}

void WireWidget::updateWire() {
}

Vec WireWidget::getOutputPos() {
}

Vec WireWidget::getInputPos() {
}

json_t *WireWidget::toJson() {
	json_t *rootJ = json_object();
	return rootJ;
}

void WireWidget::fromJson(json_t *rootJ) {
}

void WireWidget::draw(NVGcontext *vg) {
}

void WireWidget::drawPlugs(NVGcontext *vg) {
}


} // namespace rack
