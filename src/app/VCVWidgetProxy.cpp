#include "app.hpp"
#include "engine.hpp"
#include "plugin.hpp"
#include "window.hpp"
#include "compat/include/app.hpp"

namespace mirack {


VCVWidgetProxy::VCVWidgetProxy(rack::Widget *target) {
	this->target = target;
	box.size = Vec(target->box.size.x, target->box.size.y);
}

VCVWidgetProxy::~VCVWidgetProxy() {
	// // Make sure WireWidget destructors are called *before* removing `module` from the rack.
	// disconnect();
	// // Remove and delete the Module instance
	// if (module) {
	// 	engineRemoveModule(module);
	// 	delete module;
	// 	module = NULL;
	// }
}



void VCVWidgetProxy::step() {
	target->step();
}

void VCVWidgetProxy::draw(NVGcontext *vg) {
	target->draw(vg);
}

void VCVWidgetProxy::onMouseDown(EventMouseDown &e) {
	rack::EventMouseDown e2;
	e2.pos = rack::Vec(e.pos.x, e.pos.y);
	e2.button = e.button;
	target->onMouseDown(e2);
	if (e2.target)
		e.target = new VCVWidgetProxy(e2.target);
	printf("#2 %p\n",e2.target);
	// Widget::onMouseDown(e);
	// if (e.consumed)
	// 	return;

	// if (e.button == 1) {
	// 	createContextMenu();
	// }
	// e.consumed = true;
	// e.target = this;
}

void VCVWidgetProxy::onMouseMove(EventMouseMove &e) {
	// OpaqueWidget::onMouseMove(e);

	// // Don't delete the VCVModuleWidget if a TextField is focused
	// if (!gFocusedWidget) {
	// 	// Instead of checking key-down events, delete the module even if key-repeat hasn't fired yet and the cursor is hovering over the widget.
	// 	if (glfwGetKey(gWindow, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(gWindow, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
	// 		if (!windowIsModPressed() && !windowIsShiftPressed()) {
	// 			gRackWidget->deleteModule(this);
	// 			this->finalizeEvents();
	// 			delete this;
	// 			e.consumed = true;
	// 			return;
	// 		}
	// 	}
	// }
}

void VCVWidgetProxy::onHoverKey(EventHoverKey &e) {
	// switch (e.key) {
	// 	case GLFW_KEY_I: {
	// 		if (windowIsModPressed() && !windowIsShiftPressed()) {
	// 			reset();
	// 			e.consumed = true;
	// 			return;
	// 		}
	// 	} break;
	// 	case GLFW_KEY_R: {
	// 		if (windowIsModPressed() && !windowIsShiftPressed()) {
	// 			randomize();
	// 			e.consumed = true;
	// 			return;
	// 		}
	// 	} break;
	// 	case GLFW_KEY_D: {
	// 		if (windowIsModPressed() && !windowIsShiftPressed()) {
	// 			gRackWidget->cloneModule(this);
	// 			e.consumed = true;
	// 			return;
	// 		}
	// 	} break;
	// 	case GLFW_KEY_U: {
	// 		if (windowIsModPressed() && !windowIsShiftPressed()) {
	// 			disconnect();
	// 			e.consumed = true;
	// 			return;
	// 		}
	// 	} break;
	// }

	// Widget::onHoverKey(e);
}

void VCVWidgetProxy::onDragStart(EventDragStart &e) {
	rack::EventDragStart e2;
	target->onDragStart(e2);

	// dragPos = gRackWidget->lastMousePos.minus(box.pos);
}

void VCVWidgetProxy::onDragEnd(EventDragEnd &e) {
	rack::EventDragEnd e2;
	target->onDragEnd(e2);
}

void VCVWidgetProxy::onDragMove(EventDragMove &e) {
	rack::EventDragMove e2;
	e2.mouseRel = rack::Vec(e.mouseRel.x, e.mouseRel.y);
	target->onDragMove(e2);
	// if (!gRackWidget->lockModules) {
	// 	Rect newBox = box;
	// 	newBox.pos = gRackWidget->lastMousePos.minus(dragPos);
	// 	gRackWidget->requestModuleBoxNearest(this, newBox);
	// }
}

void VCVWidgetProxy::onDragEnter(EventDragEnter &e) {
	rack::EventDragEnter e2;
	target->onDragEnter(e2);
}

void VCVWidgetProxy::onDragLeave(EventDragEnter &e) {
	rack::EventDragEnter e2;
	target->onDragLeave(e2);
}



} // namespace rack