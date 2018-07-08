#include "ui.hpp"


namespace rack {

Menu::Menu() {
	box.size = Vec(0, 0);
}

Menu::~Menu() {
	setChildMenu(NULL);
}

void Menu::addChild(Widget *widget) {
	widget->autosize();
	
	widget->box.pos = Vec(0, box.size.y);
	box.size.y += widget->box.size.y;
	// Increase width based on maximum width of child
	if (widget->box.size.x > box.size.x) {
		box.size.x = widget->box.size.x;
	}

	Widget::addChild(widget);

	// Resize widths of children
	for (Widget *widget : children) {
		widget->box.size.x = box.size.x;
	}	
}

void Menu::setChildMenu(Menu *menu) {
	if (childMenu) {
		if (childMenu->parent)
			childMenu->parent->removeChild(childMenu);
		delete childMenu;
		childMenu = NULL;
	}
	if (menu) {
		childMenu = menu;
		assert(parent);
		parent->addChild(childMenu);
	}
}

void Menu::draw(NVGcontext *vg) {
	bndMenuBackground(vg, 0.0, 0.0, box.size.x, box.size.y, BND_CORNER_NONE);
	Widget::draw(vg);
}


void Menu::onScroll(EventScroll &e) {
	if (!parent)
		return;
	if (!parent->box.contains(box))
		box.pos.y += e.scrollRel.y;
	e.consumed = true;
}


} // namespace rack
