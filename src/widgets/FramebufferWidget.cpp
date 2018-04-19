#include "widgets.hpp"
#include "window.hpp"
//#include <GL/glew.h>
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"


namespace rack {


struct FramebufferWidget::Internal {
	NVGLUframebuffer *fb = NULL;
	Vec fbSize;
	Rect box;

	~Internal() {
		setFramebuffer(NULL);
	}
	void setFramebuffer(NVGLUframebuffer *fb) {
		if (this->fb)
			nvgluDeleteFramebuffer(this->fb);
		this->fb = fb;
	}
};


FramebufferWidget::FramebufferWidget() {
	canCache = true;
	oversample = 1.0;
	internal = new Internal();
}

FramebufferWidget::~FramebufferWidget() {
	delete internal;
}

void FramebufferWidget::draw(NVGcontext *vg) {
	Widget::draw(vg);
}

int FramebufferWidget::getImageHandle() {
	if (!internal->fb)
		return -1;
	return internal->fb->image;
}

void FramebufferWidget::onZoom(EventZoom &e) {
	dirty += 2;
	Widget::onZoom(e);
}


} // namespace rack
