#include "ui.hpp"


namespace rack {

//TODO: no need to do all of this on resize
void List::onResize() {
	// Set positions of children
	box.size.y = 0.0;
	for (Widget *child : children) {
		if (!child->visible)
			continue;
		// Increment height, set position of child
		child->box.pos = Vec(0.0, box.size.y);
		box.size.y += child->box.size.y;
		// Resize width of child
		child->box.size.x = box.size.x;
	}

	Widget::onResize();	
}


} // namespace rack
