#include "widgets.hpp"
#include "app.hpp"
#include "window.hpp"
#include <algorithm>

#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

namespace rack {

Widget::~Widget() {
	// You should only delete orphaned widgets
	assert(!parent);
	// Stop dragging and hovering this widget
	if (gHoveredWidget == this) gHoveredWidget = NULL;
	if (gDraggedWidget == this) gDraggedWidget = NULL;
	if (gDragHoveredWidget == this) gDragHoveredWidget = NULL;
	if (gFocusedWidget == this) gFocusedWidget = NULL;
	if (gTempWidget == this) gTempWidget = NULL;
	clearChildren();
}

Rect Widget::getChildrenBoundingBox() {
	Rect bound;
	for (Widget *child : children) {
		if (child == children.front()) {
			bound = child->box;
		}
		else {
			bound = bound.expand(child->box);
		}
	}
	return bound;
}

Vec Widget::getRelativeOffset(Vec v, Widget *relative) {
	if (this == relative) {
		return v;
	}
	v = v.plus(box.pos);
	if (parent) {
		v = parent->getRelativeOffset(v, relative);
	}
	return v;
}

Rect Widget::getViewport(Rect r) {
	Rect bound;
	if (parent) {
		bound = parent->getViewport(box);
	}
	else {
		bound = box;
	}
	bound.pos = bound.pos.minus(box.pos);
	return r.clamp(bound);
}

void Widget::addChild(Widget *widget) {
	assert(!widget->parent);
	widget->parent = this;
	children.push_back(widget);
}

void Widget::removeChild(Widget *widget) {
	assert(widget->parent == this);
	auto it = std::find(children.begin(), children.end(), widget);
	if (it != children.end()) {
		children.erase(it);
		widget->parent = NULL;
	}
}

void Widget::clearChildren() {
	for (Widget *child : children) {
		child->parent = NULL;
		delete child;
	}
	children.clear();
}

void Widget::finalizeEvents() {
	// Stop dragging and hovering this widget
	if (gHoveredWidget == this) {
		EventMouseLeave e;
		gHoveredWidget->onMouseLeave(e);
		gHoveredWidget = NULL;
	}
	if (gDraggedWidget == this) {
		EventDragEnd e;
		gDraggedWidget->onDragEnd(e);
		gDraggedWidget = NULL;
	}
	if (gDragHoveredWidget == this) {
		gDragHoveredWidget = NULL;
	}
	if (gFocusedWidget == this) {
		EventDefocus e;
		gFocusedWidget->onDefocus(e);
		gFocusedWidget = NULL;
	}
	for (Widget *child : children) {
		child->finalizeEvents();
	}
}

void Widget::step() {
	for (Widget *child : children) {
		child->step();
	}
}

void Widget::draw(NVGcontext *vg) {
	for (Widget *child : children) {
		if (!child->visible)
			continue;
		nvgSave(vg);
		nvgTranslate(vg, child->box.pos.x, child->box.pos.y);
		child->drawCachedOrFresh(vg);
		nvgRestore(vg);
		child->needsRender--;// = false;
	}
}

void Widget::drawCachedOrFresh(NVGcontext *vg) {
	if(!canCache)
	{ 
		draw(vg);
		return;
	}

// Get world transform
	float xform[6];
	nvgCurrentTransform(vg, xform);
	// Skew and rotate is not supported
	// assert(fabsf(xform[1]) < 1e-6);
	// assert(fabsf(xform[2]) < 1e-6);
	Vec s = Vec(xform[0], xform[3]);
	Vec b = Vec(xform[4], xform[5]);
	Vec bi = b.floor();
	Vec bf = b.minus(bi);

	// Render to framebuffer
	if (dirty) {
		dirty = false;

		fbBox = getChildrenBoundingBox();
		fbBox.pos = fbBox.pos.mult(s).floor();
		fbBox.size = fbBox.size.mult(s).ceil().plus(Vec(1, 1));

		Vec fbSize2 = fbBox.size.mult(gPixelRatio * oversample);

		if (!fbSize2.isFinite())
			return;
		if (fbSize2.isZero())
			return;

		if (!fbSize2.isEqual(fbSize))
		{
			fbSize = fbSize2;
			// info("rendering framebuffer %f %f", fbSize.x, fbSize.y);
			// Delete old one first to free up GPU memory
			if (fb)
				nvgluDeleteFramebuffer(fb);
			// Create a framebuffer from the main nanovg context. We will draw to this in the secondary nanovg context.
			fb = nvgluCreateFramebuffer(gVg, fbSize.x, fbSize.y, 0);
			if (!fb)
				return;
		}

		nvgluBindFramebuffer(fb);
		glViewport(0.0, 0.0, fbSize.x, fbSize.y);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(gFramebufferVg, fbSize.x, fbSize.y, gPixelRatio * oversample);

		nvgScale(gFramebufferVg, gPixelRatio * oversample, gPixelRatio * oversample);
		// Use local scaling
		nvgTranslate(gFramebufferVg, bf.x, bf.y);
		nvgTranslate(gFramebufferVg, -fbBox.pos.x, -fbBox.pos.y);
		nvgScale(gFramebufferVg, s.x, s.y);
		Widget::draw(gFramebufferVg);

		nvgEndFrame(gFramebufferVg);
		nvgluBindFramebuffer(NULL);
	}

	if (!fb)
		return;

	// Draw framebuffer image, using world coordinates
	nvgSave(vg);
	nvgResetTransform(vg);
	nvgTranslate(vg, bi.x, bi.y);

	nvgBeginPath(vg);
	nvgRect(vg, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y);
	NVGpaint paint = nvgImagePattern(vg, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y, 0.0, fb->image, 1.0);
	nvgFillPaint(vg, paint);
	nvgFill(vg);

	// For debugging the bounding box of the framebuffer
	// nvgStrokeWidth(vg, 2.0);
	// nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 128));
	// nvgStroke(vg);

	nvgRestore(vg);
}

#define RECURSE_EVENT_POSITION(_method) { \
	Vec pos = e.pos; \
	for (auto it = children.rbegin(); it != children.rend(); it++) { \
		Widget *child = *it; \
		if (!child->visible) \
			continue; \
		if (child->box.contains(pos)) { \
			e.pos = pos.minus(child->box.pos); \
			child->_method(e); \
			if (e.consumed) \
				break; \
		} \
	} \
	e.pos = pos; \
}


void Widget::onMouseDown(EventMouseDown &e) {
	RECURSE_EVENT_POSITION(onMouseDown);
}

void Widget::onMouseUp(EventMouseUp &e) {
	RECURSE_EVENT_POSITION(onMouseUp);
}

void Widget::onMouseMove(EventMouseMove &e) {
	RECURSE_EVENT_POSITION(onMouseMove);
}

void Widget::onHoverKey(EventHoverKey &e) {
	RECURSE_EVENT_POSITION(onHoverKey);
}

void Widget::onScroll(EventScroll &e) {
	RECURSE_EVENT_POSITION(onScroll);
}

void Widget::onPathDrop(EventPathDrop &e) {
	RECURSE_EVENT_POSITION(onPathDrop);
}

void Widget::onZoom(EventZoom &e) {
	for (auto it = children.rbegin(); it != children.rend(); it++) {
		Widget *child = *it;
		child->onZoom(e);
	}
}

} // namespace rack
