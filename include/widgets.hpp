#pragma once
#include <list>
#include <memory>

#include "nanovg.h"
#include "nanosvg.h"

#include "util/common.hpp"
#include "events.hpp"

struct NVGLUframebuffer;

namespace rack {

////////////////////
// resources
////////////////////

// Constructing these directly will load from the disk each time. Use the load() functions to load from disk and cache them as long as the shared_ptr is held.
// Implemented in window.cpp

struct Font {
	int handle;
	Font(const std::string &filename);
	~Font();
	static std::shared_ptr<Font> load(const std::string &filename);
};

struct Image {
	int handle;
	Image(const std::string &filename);
	~Image();
	static std::shared_ptr<Image> load(const std::string &filename);
};

struct SVG {
	int image;
	Vec size;
	SVG(const std::string &filename);
	~SVG();
	static std::shared_ptr<SVG> load(const std::string &filename);
};


////////////////////
// Base widget
////////////////////

struct Widget;

/*struct PosWrapper : Vec {
	Widget *widget;

	PosWrapper(Widget *_widget, Vec v) {
		widget = _widget;
		x = v.x;
		y = v.y;
	};

	PosWrapper& operator=(Vec r);
};*/

struct SizeWrapper : Vec {
	Widget *widget;

	//TODO: should also intercept individual x/y changes (only X really)

	SizeWrapper(Widget *_widget, Vec v) {
		widget = _widget;
		x = v.x;
		y = v.y;
	};

	SizeWrapper& operator=(Vec r);
	SizeWrapper& operator=(SizeWrapper another);
};

struct BoxWrapper {
	Widget *widget;
	
	Vec pos;
	SizeWrapper size;

	BoxWrapper(Widget *_widget, Rect r) : pos(r.pos), size(_widget, r.size) {
		widget = _widget;
	};

	BoxWrapper& operator=(Rect r);
	BoxWrapper& operator=(BoxWrapper another);

	operator Rect() {
		return Rect(pos, size);
	};

	/** Returns whether this Rect contains an entire point, inclusive on the top/left, non-inclusive on the bottom/right */
	bool contains(Vec v) {
		return pos.x <= v.x && v.x < pos.x + size.x
			&& pos.y <= v.y && v.y < pos.y + size.y;
	}
	/** Returns whether this Rect contains an entire Rect */
	bool contains(Rect r) {
		return pos.x <= r.pos.x && r.pos.x + r.size.x <= pos.x + size.x
			&& pos.y <= r.pos.y && r.pos.y + r.size.y <= pos.y + size.y;
	}
	/** Returns whether this Rect overlaps with another Rect */
	bool intersects(Rect r) {
		return (pos.x + size.x > r.pos.x && r.pos.x + r.size.x > pos.x)
			&& (pos.y + size.y > r.pos.y && r.pos.y + r.size.y > pos.y);
	}
	bool isEqual(Rect r) {
		return pos.isEqual(r.pos) && size.isEqual(r.size);
	}
	Vec getCenter() {
		return pos.plus(size.mult(0.5f));
	}
	Vec getTopRight() {
		return pos.plus(Vec(size.x, 0.0f));
	}
	Vec getBottomLeft() {
		return pos.plus(Vec(0.0f, size.y));
	}
	Vec getBottomRight() {
		return pos.plus(size);
	}
	/** Clamps the edges of the rectangle to fit within a bound */
	Rect clamp(Rect bound) {
		Rect r;
		r.pos.x = clamp2(pos.x, bound.pos.x, bound.pos.x + bound.size.x);
		r.pos.y = clamp2(pos.y, bound.pos.y, bound.pos.y + bound.size.y);
		r.size.x = rack::clamp(pos.x + size.x, bound.pos.x, bound.pos.x + bound.size.x) - r.pos.x;
		r.size.y = rack::clamp(pos.y + size.y, bound.pos.y, bound.pos.y + bound.size.y) - r.pos.y;
		return r;
	}
	/** Nudges the position to fix inside a bounding box */
	Rect nudge(Rect bound) {
		Rect r;
		r.size = size;
		r.pos.x = clamp2(pos.x, bound.pos.x, bound.pos.x + bound.size.x - size.x);
		r.pos.y = clamp2(pos.y, bound.pos.y, bound.pos.y + bound.size.y - size.y);
		return r;
	}
	/** Expands this Rect to contain `other` */
	Rect expand(Rect other) {
		Rect r;
		r.pos.x = min(pos.x, other.pos.x);
		r.pos.y = min(pos.y, other.pos.y);
		r.size.x = max(pos.x + size.x, other.pos.x + other.size.x) - r.pos.x;
		r.size.y = max(pos.y + size.y, other.pos.y + other.size.y) - r.pos.y;
		return r;
	}
	/** Returns a Rect with its position set to zero */
	Rect zeroPos() {
		Rect r;
		r.size = size;
		return r;
	}
	Rect grow(Vec delta) {
		Rect r;
		r.pos = pos.minus(delta);
		r.size = size.plus(delta.mult(2.f));
		return r;
	}	
};

struct DirtyWrapper {
	Widget *widget;
	bool dirty;

	DirtyWrapper(Widget *_widget, bool _dirty) {
		widget = _widget;
		dirty = _dirty;
	}

	DirtyWrapper& operator =(bool _dirty);

	operator bool() {
		return dirty;
	};
};

/** A node in the 2D scene graph
Never inherit from Widget directly. Instead, inherit from VirtualWidget declared below.
*/
struct Widget {
	/** Stores position and size */
	Rect box = Rect(Vec(), Vec(INFINITY, INFINITY));
	Rect lastBox;
	Widget *parent = NULL;
	std::list<Widget*> children;
	bool visible = true;
	bool canSquash = false;
	bool canCache = false;
	bool canGrowHitBox = false;

	NVGLUframebuffer *fb = NULL;
	Vec fbSize;
	Rect fbBox;
	NVGpaint fbPaint;
	DirtyWrapper dirty;
	float oversample = 1;
	
	Widget();
	virtual ~Widget();

	virtual Rect getChildrenBoundingBox();
	/**  Returns `v` transformed into the coordinate system of `relative` */
	virtual Vec getRelativeOffset(Vec v, Widget *relative);
	/** Returns `v` transformed into world coordinates */
	Vec getAbsoluteOffset(Vec v) {
		return getRelativeOffset(v, NULL);
	}
	/** Returns a subset of the given Rect bounded by the box of this widget and all ancestors */
	virtual Rect getViewport(Rect r);

	template <class T>
	T *getAncestorOfType() {
		if (!parent) return NULL;
		T *p = dynamic_cast<T*>(parent);
		if (p) return p;
		return parent->getAncestorOfType<T>();
	}

	template <class T>
	T *getFirstDescendantOfType() {
		for (Widget *child : children) {
			T *c = dynamic_cast<T*>(child);
			if (c) return c;
			c = child->getFirstDescendantOfType<T>();
			if (c) return c;
		}
		return NULL;
	}

	/** Adds widget to list of children.
	Gives ownership of widget to this widget instance.
	*/
	void addChild(Widget *widget);
	/** Removes widget from list of children if it exists.
	Does not delete widget but transfers ownership to caller
	Silently fails if widget is not a child
	*/
	void removeChild(Widget *widget);
	void clearChildren();
	/** Recursively finalizes event start/end pairs as needed */
	void finalizeEvents();

	/** Advances the module by one frame */
	virtual void step();
	/** Draws to NanoVG context */
	virtual void draw(NVGcontext *vg);
	void drawCachedOrFresh(NVGcontext *vg);
	void ensureCached(NVGcontext *vg);

	Rect getHitBox();
	virtual void autosize() {};

	// Events

	/** Called when a mouse button is pressed over this widget
	0 for left, 1 for right, 2 for middle.
	Return `this` to accept the event.
	Return NULL to reject the event and pass it to the widget behind this one.
	*/
	virtual void onMouseDown(EventMouseDown &e);
	virtual void onMouseUp(EventMouseUp &e);
	/** Called on every frame, even if mouseRel = Vec(0, 0) */
	virtual void onMouseMove(EventMouseMove &e);
	virtual void onHoverKey(EventHoverKey &e);
	/** Called when this widget begins responding to `onMouseMove` events */
	virtual void onMouseEnter(EventMouseEnter &e) {}
	/** Called when another widget begins responding to `onMouseMove` events */
	virtual void onMouseLeave(EventMouseLeave &e) {}
	virtual void onFocus(EventFocus &e) {}
	virtual void onDefocus(EventDefocus &e) {}
	virtual void onText(EventText &e) {}
	virtual void onKey(EventKey &e) {}
	virtual void onScroll(EventScroll &e);

	/** Called when a widget responds to `onMouseDown` for a left button press */
	virtual void onDragStart(EventDragStart &e) {}
	/** Called when the left button is released and this widget is being dragged */
	virtual void onDragEnd(EventDragEnd &e) {}
	/** Called when a widget responds to `onMouseMove` and is being dragged */
	virtual void onDragMove(EventDragMove &e) {}
	/** Called when a widget responds to `onMouseUp` for a left button release and a widget is being dragged */
	virtual void onDragEnter(EventDragEnter &e) {}
	virtual void onDragLeave(EventDragEnter &e) {}
	virtual void onDragDrop(EventDragDrop &e) {}
	virtual void onPathDrop(EventPathDrop &e);

	virtual void onAction(EventAction &e) {}
	virtual void onChange(EventChange &e) {}
	virtual void onZoom(EventZoom &e);
	virtual void onResize();

	/** Helper function for creating and initializing a Widget with certain arguments (in this case just the position).
	In this project, you will find this idiom everywhere, as an easier alternative to constructor arguments, for building a Widget (or a subclass) with a one-liner.
	Example:
		addChild(Widget::create<SVGWidget>(Vec(0, 0)))
	*/
	template <typename T = Widget>
	static T *create(Vec pos) {
		T *o = new T();
		o->box.pos = pos;
		return o;
	}
};

template <typename T>
struct FieldWrapper : T {
	Widget *widget;

	FieldWrapper(Widget *_widget, T v) {
		widget = _widget;
	};

	FieldWrapper& operator=(T v) {
		T::operator=(v);
		widget->dirty = true;

		return *this;
	};

	FieldWrapper& operator=(FieldWrapper<T> &another) {
		T::operator=(another);
		widget->dirty = true;

		return *this;		
	};
};

/** Instead of inheriting from Widget directly, inherit from VirtualWidget to guarantee that only one copy of Widget's member variables are used by each instance of the Widget hierarchy.
*/
struct VirtualWidget : virtual Widget {};

struct TransformWidget : VirtualWidget {
	/** The transformation matrix */
	float transform[6];
	TransformWidget();
	Rect getChildrenBoundingBox() override;
	void identity();
	void translate(Vec delta);
	void rotate(float angle);
	void scale(Vec s);
	void draw(NVGcontext *vg) override;
};

struct ZoomWidget : VirtualWidget {
	float zoom = 1.0;
	Vec getRelativeOffset(Vec v, Widget *relative) override;
	Rect getViewport(Rect r) override;
	void setZoom(float zoom);
	void draw(NVGcontext *vg) override;
	void onMouseDown(EventMouseDown &e) override;
	void onMouseUp(EventMouseUp &e) override;
	void onMouseMove(EventMouseMove &e) override;
	void onHoverKey(EventHoverKey &e) override;
	void onScroll(EventScroll &e) override;
	void onPathDrop(EventPathDrop &e) override;
};

////////////////////
// Trait widgets
////////////////////

/** Widget that does not respond to events */
struct TransparentWidget : VirtualWidget {
	void onMouseDown(EventMouseDown &e) override {}
	void onMouseUp(EventMouseUp &e) override {}
	void onMouseMove(EventMouseMove &e) override {}
	void onScroll(EventScroll &e) override {}
};

/** Widget that automatically responds to all mouse events but gives a chance for children to respond instead */
struct OpaqueWidget : VirtualWidget {
	void onMouseDown(EventMouseDown &e) override {
		Widget::onMouseDown(e);
		if (!e.target)
			e.target = this;
		e.consumed = true;
	}
	void onMouseUp(EventMouseUp &e) override {
		Widget::onMouseUp(e);
		if (!e.target)
			e.target = this;
		e.consumed = true;
	}
	void onMouseMove(EventMouseMove &e) override {
		Widget::onMouseMove(e);
		if (!e.target)
			e.target = this;
		e.consumed = true;
	}
	void onScroll(EventScroll &e) override {
		Widget::onScroll(e);
		e.consumed = true;
	}
};

struct SpriteWidget : VirtualWidget {
	Vec spriteOffset;
	Vec spriteSize;
	std::shared_ptr<Image> spriteImage;
	int index = 0;
	void draw(NVGcontext *vg) override;
};

struct SVGWidget;

struct SVGWrapper {
	SVGWidget *widget;
	std::shared_ptr<SVG> svg;

	SVGWrapper(SVGWidget *_widget) {
		widget = _widget;
	}

	SVGWrapper& operator =(std::shared_ptr<SVG> svg);
	SVGWrapper& operator =(std::nullptr_t svg);

	operator SVG*() {
		return svg.get();
	};
	SVG* operator ->() {
		return svg.operator->();
	};
};

struct SVGWidget : VirtualWidget {
	SVGWrapper svg;

	SVGWidget();
	/** Sets the box size to the svg image size */
	void wrap();
	/** Sets and wraps the SVG */
	void setSVG(std::shared_ptr<SVG> svg);
	void draw(NVGcontext *vg) override;
};

struct FramebufferWidget : VirtualWidget {
	FramebufferWidget() {
		canCache = true;
	};
};

/** A Widget representing a float value */
struct QuantityWidget : VirtualWidget {
	float value = 0.0;
	float minValue = 0.0;
	float maxValue = 1.0;
	float defaultValue = 0.0;
	std::string label;
	/** Include a space character if you want a space after the number, e.g. " Hz" */
	std::string unit;
	/** The decimal place to round for displaying values.
	A precision of 2 will display as "1.00" for example.
	*/
	int precision = 2;

	QuantityWidget();
	void setValue(float value);
	void setLimits(float minValue, float maxValue);
	void setDefaultValue(float defaultValue);
	/** Generates the display value */
	std::string getText();
};


////////////////////
// globals
////////////////////

extern Widget *gHoveredWidget;
extern Widget *gDraggedWidget;
extern Widget *gDragHoveredWidget;
extern Widget *gFocusedWidget;
extern Widget *gTempWidget;


} // namespace rack
