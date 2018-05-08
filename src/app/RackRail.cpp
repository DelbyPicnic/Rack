#include "app.hpp"
#include "asset.hpp"
#include "window.hpp"

#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"


namespace rack {

static SVGWidget *sw;

void RackRail::step() {
	if (sw)
		return;

	sw = new SVGWidget();
	sw->setSVG(SVG::load(assetGlobal("res/RailDouble.svg")));
	
	// if (isNear(gPixelRatio, 1.0))
	// 	oversample = 2.0;
	Vec fbSize = sw->box.size.mult(gPixelRatio * oversample);
	NVGcontext *vg = gVg;
	
	sw->fb = nvgluCreateFramebuffer(vg, fbSize.x, fbSize.y, NVG_IMAGE_REPEATX);
	nvgluBindFramebuffer(sw->fb);
	glViewport(0.0, 0.0, fbSize.x, fbSize.y);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(vg, fbSize.x, fbSize.y, gPixelRatio * oversample);

	nvgReset(vg);
	nvgScale(vg, gPixelRatio * oversample, gPixelRatio * oversample);
	sw->draw(vg);
	nvgEndFrame(vg);
	nvgluBindFramebuffer(NULL);
}

void RackRail::draw(NVGcontext *vg) {
	const float railHeight = RACK_GRID_WIDTH;
	nvgSave(vg);
	nvgTranslate(vg, 0, -railHeight);

	for (float railY = 0; railY < gRackWidget->box.size.y; railY += RACK_GRID_HEIGHT) {
		nvgBeginPath(vg);
		nvgRect(vg, 0.0, 0.0, gRackWidget->box.size.x, sw->box.size.y);
		nvgFillPaint(vg, nvgImagePattern(vg, 0, 0, sw->box.size.x, sw->box.size.y, 0.0, sw->fb->image, 1.0));
		nvgFill(vg);
		nvgTranslate(vg, 0, RACK_GRID_HEIGHT);
	}
	nvgRestore(vg);
}


} // namespace rack
