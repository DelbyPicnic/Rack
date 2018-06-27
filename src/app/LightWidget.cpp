#include "app.hpp"
#include "util/color.hpp"


namespace mirack {


void LightWidget::draw(NVGcontext *vg) {
	//drawLight(vg);
	//drawHalo(vg);
}

void LightWidget::drawLight(NVGcontext *vg) {
	float radius = box.size.x / 2.0;

	nvgBeginPath(vg);
	nvgCircle(vg, radius, radius, radius);

	// Background
	if (color.a < 1.f)
	{
		nvgFillColor(vg, bgColor);
		nvgFill(vg);
	}

	// Foreground
	if (color.a)
	{
		nvgFillColor(vg, color);
		nvgFill(vg);
	}

	// Border
	if (borderColor.a)
	{
		nvgStrokeWidth(vg, 0.5);
		nvgStrokeColor(vg, borderColor);
		nvgStroke(vg);
	}
}

void LightWidget::drawHalo(NVGcontext *vg) {
	float radius = box.size.x / 2.0;
	float oradius = radius + 15.0;

	nvgBeginPath(vg);
	nvgRect(vg, radius - oradius, radius - oradius, 2*oradius, 2*oradius);

	NVGpaint paint;
	NVGcolor icol = colorMult(color, 0.08);
	NVGcolor ocol = nvgRGB(0, 0, 0);
	paint = nvgRadialGradient(vg, radius, radius, radius, oradius, icol, ocol);
	nvgFillPaint(vg, paint);
	nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
	nvgFill(vg);
}


} // namespace mirack
