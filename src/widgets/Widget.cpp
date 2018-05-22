#include "widgets.hpp"
#include "app.hpp"
#include "window.hpp"
#include "settings.hpp"
#include <algorithm>

#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

namespace rack {

/*PosWrapper& PosWrapper::operator=(Vec v) {
	x = v.x;
	y = v.y;

	return *this;
};*/

SizeWrapper& SizeWrapper::operator=(Vec v) {
	bool changed = !isEqual(v);

	x = v.x;
	y = v.y;
	
	if (changed)
		widget->onResize();	
	return *this;
};

SizeWrapper& SizeWrapper::operator=(SizeWrapper another) {
	bool changed = !isEqual(another);

	x = another.x;
	y = another.y;

	if (changed)
		widget->onResize();	
	return *this;
};

BoxWrapper& BoxWrapper::operator=(Rect r) {
	pos = r.pos;
	size = r.size;

	return *this;
};

BoxWrapper& BoxWrapper::operator=(BoxWrapper another) {
	pos = another.pos;
	size = another.size;

	return *this;
};

DirtyWrapper& DirtyWrapper::operator =(bool _dirty) {
	dirty = _dirty;
	if (dirty && widget) {
		Widget *p = widget;
		while((p=p->parent)) {
			if (p->dirty.dirty)
				break;
			p->dirty.dirty = true;
		}
	}
	return *this;
}

Widget::Widget() : /*box(this, Rect(Vec(), Vec(INFINITY, INFINITY))),*/ dirty(this, true) {

}

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
	if (!box.size.isEqual(lastBox.size)) {
		onResize();
		lastBox.size = box.size;
	}

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
	}
}

void Widget::ensureCached(NVGcontext *vg) {
	if (!dirty)
		return;

	for (Widget *child : children)
		child->ensureCached(vg);

	if (canCache && dirty) {
		fbBox = Rect(Vec(0,0), box.size);//children.size() ? getChildrenBoundingBox() : Rect(Vec(0,0), box.size);
		fbBox.size = fbBox.size.ceil();

		// Small details draw poorly at low DPI, so oversample when drawing to the framebuffer
		if (isNear(gPixelRatio, 1.0))
			oversample = 2.0;

		Vec fbSize2 = fbBox.size.mult(gPixelRatio * oversample);

		if (!fbSize2.isFinite())
			return;
		if (fbSize2.isZero())
			return;

		if (!fbSize2.isEqual(fbSize)) {
			fbSize = fbSize2;
			// info("rendering framebuffer %f %f", fbSize.x, fbSize.y);
			// Delete old one first to free up GPU memory
			if (fb)
				nvgluDeleteFramebuffer(fb);
			// Create a framebuffer from the main nanovg context. We will draw to this in the secondary nanovg context.
			fb = nvgluCreateFramebuffer(vg, fbSize.x, fbSize.y, 0); //gVg
			if (!fb)
				return;
		}

		// printf("drawing %p %f %f %f %f   %f %f %f %f  %f %f\n", this, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y, box.pos.x, box.pos.y, box.size.x, box.size.y, gPixelRatio, oversample);
		nvgluBindFramebuffer(fb);
		glViewport(0.0, 0.0, fbSize.x, fbSize.y);
		// glClearColor((float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, 1);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vg, fbSize.x, fbSize.y, gPixelRatio * oversample);

		nvgReset(vg);
		nvgScale(vg, gPixelRatio * oversample, gPixelRatio * oversample);
		draw(vg);
		nvgEndFrame(vg);
		// printf("drawing end %p %f %f %f %f   %f %f %f %f  %f %f\n", this, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y, box.pos.x, box.pos.y, box.size.x, box.size.y, gPixelRatio, oversample);
		nvgluBindFramebuffer(NULL);

		// fbPaint = nvgImagePattern(vg, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y, 0.0, fb->image, 1.0);		
	}

	dirty = false;	
}

void Widget::drawCachedOrFresh(NVGcontext *vg) {
	if(!canCache) { 
		draw(vg);
		return;
	}

	if (!fb)
		return;

	// Draw framebuffer image, using world coordinates
	nvgSave(vg);
	// printf("presenting framebuffer %f %f %f %f %d %p\n", fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y, fb->image, vg);
	nvgBeginPath(vg);
	nvgRect(vg, fbBox.pos.x, fbBox.pos.y, fbBox.size.x, fbBox.size.y);

	nvgTextureQuads(vg, fb->image);

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

// For simplification we assume that components with non-zero hitMargins will consume the event
#define RECURSE_EVENT_POSITION_WITH_MARGINS(_method) { \
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
	if (largerHitBoxes && !e.consumed) { \
		Widget* target = NULL; \
		for (auto it = children.rbegin(); it != children.rend(); it++) { \
			Widget *child = *it; \
			if (!child->visible) \
				continue; \
			if (child->getHitBox().contains(pos)) { \
				if (!target || child->box.getCenter().minus(pos).norm() < target->box.getCenter().minus(pos).norm()) \
					target = child; \
			} \
		} \
		if (target) { \
			e.pos = pos.minus(target->box.pos); \
			target->_method(e); \
		} \
	} \
	e.pos = pos; \
}

void Widget::onMouseDown(EventMouseDown &e) {
	RECURSE_EVENT_POSITION_WITH_MARGINS(onMouseDown);
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

void Widget::onResize() {
	if (canCache)
		dirty = true;
}

Rect Widget::getHitBox() {
	if (!canGrowHitBox)
		return box;

	float min = 32.0f / gRackScene->zoomWidget->zoom;

	Vec grow;
	if (box.size.x < min)
		grow.x = (min - box.size.x) * 0.5;
	if (box.size.y < min)
		grow.y = (min - box.size.y) * 0.5;

	return box.grow(grow);
}

} // namespace rack
