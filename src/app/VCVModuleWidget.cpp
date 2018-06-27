#include "app.hpp"
#include "engine.hpp"
#include "plugin.hpp"
#include "window.hpp"
#include "compat/include/app.hpp"
#include "compat/include/engine.hpp"

namespace mirack {


VCVModuleWidget::VCVModuleWidget(rack::ModuleWidget *mw) : ModuleWidget(NULL) {
	this->mw = mw;
	box.size = Vec(mw->box.size.x, mw->box.size.y);

	auto *mp = new VCVModuleProxy(mw->module);

	for (rack::Widget *w : mw->children) {
		if (rack::Port *p = dynamic_cast<rack::Port*>(w)) {
			// p->parent->removeChild(p);
			auto *pp = new VCVPortProxy(p);
			pp->box = Rect(Vec(p->box.pos.x, p->box.pos.y), Vec(p->box.size.x, p->box.size.y));
			pp->type = (mirack::Port::PortType)((int)p->type);
			pp->portId = p->portId;
			pp->module = mp;
			addChild(pp);
		}
	}

	engineAddModule(mp);
}

VCVModuleWidget::~VCVModuleWidget() {
	// // Make sure WireWidget destructors are called *before* removing `module` from the rack.
	// disconnect();
	// // Remove and delete the Module instance
	// if (module) {
	// 	engineRemoveModule(module);
	// 	delete module;
	// 	module = NULL;
	// }
}


json_t *VCVModuleWidget::toJson() {
	return mw->toJson();
}

void VCVModuleWidget::fromJson(json_t *rootJ) {
	mw->fromJson(rootJ);
	box.pos = Vec(mw->box.pos.x, mw->box.pos.y);
}

void VCVModuleWidget::disconnect() {
	for (Port *input : inputs) {
		gRackWidget->wireContainer->removeAllWires(input);
	}
	for (Port *output : outputs) {
		gRackWidget->wireContainer->removeAllWires(output);
	}
}

void VCVModuleWidget::create() {
}

void VCVModuleWidget::_delete() {
}

void VCVModuleWidget::reset() {
	for (ParamWidget *param : params) {
		param->reset();
	}
	if (module) {
		module->onReset();
	}
}

void VCVModuleWidget::randomize() {
	for (ParamWidget *param : params) {
		param->randomize();
	}
	if (module) {
		module->onRandomize();
	}
}

void VCVModuleWidget::step() {
	mw->step();
}

void VCVModuleWidget::draw(NVGcontext *vg) {
	mw->draw(vg);
	// Widget::draw(vg);
	// nvgScissor(vg, 0, 0, box.size.x, box.size.y);
	// Widget::draw(vg);

	// // CPU meter
	// if (module && gPowerMeter) {
	// 	nvgBeginPath(vg);
	// 	nvgRect(vg,
	// 		0, box.size.y - 20,
	// 		55, 20);
	// 	nvgFillColor(vg, nvgRGBAf(0, 0, 0, 0.5));
	// 	nvgFill(vg);

	// 	std::string cpuText = stringf("%.0f mS", module->cpuTime * 1000.f);
	// 	nvgFontFaceId(vg, gGuiFont->handle);
	// 	nvgFontSize(vg, 12);
	// 	nvgFillColor(vg, nvgRGBf(1, 1, 1));
	// 	nvgText(vg, 10.0, box.size.y - 6.0, cpuText.c_str(), NULL);

	// 	float p = clamp(module->cpuTime, 0.f, 1.f);
	// 	nvgBeginPath(vg);
	// 	nvgRect(vg,
	// 		0, (1.f - p) * box.size.y,
	// 		5, p * box.size.y);
	// 	nvgFillColor(vg, nvgRGBAf(1, 0, 0, 1.0));
	// 	nvgFill(vg);
	// }

	// nvgResetScissor(vg);
}

void VCVModuleWidget::drawShadow(NVGcontext *vg) {
	// nvgBeginPath(vg);
	// float r = 20; // Blur radius
	// float c = 20; // Corner radius
	// Vec b = Vec(-10, 30); // Offset from each corner
	// nvgRect(vg, b.x - r, b.y - r, box.size.x - 2*b.x + 2*r, box.size.y - 2*b.y + 2*r);
	// NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.2);
	// NVGcolor transparentColor = nvgRGBAf(0, 0, 0, 0);
	// nvgFillPaint(vg, nvgBoxGradient(vg, b.x, b.y, box.size.x - 2*b.x, box.size.y - 2*b.y, c, r, shadowColor, transparentColor));
	// nvgFill(vg);
}

void VCVModuleWidget::onMouseDown(EventMouseDown &e) {
	Widget::onMouseDown(e);
	if (e.consumed && e.target != this)
		return;

	rack::EventMouseDown e2;
	e2.pos = rack::Vec(e.pos.x, e.pos.y);
	e2.button = e.button;
	mw->onMouseDown(e2);
	e.consumed = e2.consumed;
	if (e2.target)
		if (e2.target == mw)
			e.target = this;
		else
			e.target = new VCVWidgetProxy(e2.target);
	printf("#1 %p\n",e.target);
	// if (e.consumed)
	// 	return;

	// if (e.button == 1) {
	// 	createContextMenu();
	// }
	// e.consumed = true;
	// e.target = this;
}

void VCVModuleWidget::onMouseMove(EventMouseMove &e) {
	OpaqueWidget::onMouseMove(e);

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

void VCVModuleWidget::onHoverKey(EventHoverKey &e) {
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

void VCVModuleWidget::onDragStart(EventDragStart &e) {
	// rack::EventDragStart e2;
	// mw->onDragStart(e2);

	dragPos = gRackWidget->lastMousePos.minus(box.pos);
}

void VCVModuleWidget::onDragEnd(EventDragEnd &e) {
}

void VCVModuleWidget::onDragMove(EventDragMove &e) {
	// if (!gRackWidget->lockModules)
	{
		Rect newBox = box;
		newBox.pos = gRackWidget->lastMousePos.minus(dragPos);
		gRackWidget->requestModuleBoxNearest(this, newBox);
	}
}

void VCVModuleWidget::onDragEnter(EventDragEnter &e) {
	rack::EventDragEnter e2;
	mw->onDragEnter(e2);
}

void VCVModuleWidget::onDragLeave(EventDragEnter &e) {
	rack::EventDragEnter e2;
	mw->onDragLeave(e2);
}



// struct DisconnectMenuItem : MenuItem {
// 	VCVModuleWidget *VCVModuleWidget;
// 	void onAction(EventAction &e) override {
// 		VCVModuleWidget->disconnect();
// 	}
// };

// struct ResetMenuItem : MenuItem {
// 	VCVModuleWidget *VCVModuleWidget;
// 	void onAction(EventAction &e) override {
// 		VCVModuleWidget->reset();
// 	}
// };

// struct RandomizeMenuItem : MenuItem {
// 	VCVModuleWidget *VCVModuleWidget;
// 	void onAction(EventAction &e) override {
// 		VCVModuleWidget->randomize();
// 	}
// };

// struct CloneMenuItem : MenuItem {
// 	VCVModuleWidget *VCVModuleWidget;
// 	void onAction(EventAction &e) override {
// 		gRackWidget->cloneModule(VCVModuleWidget);
// 	}
// };

// struct DeleteMenuItem : MenuItem {
// 	VCVModuleWidget *VCVModuleWidget;
// 	void onAction(EventAction &e) override {
// 		gRackWidget->deleteModule(VCVModuleWidget);
// 		VCVModuleWidget->finalizeEvents();
// 		delete VCVModuleWidget;
// 	}
// };

Menu *VCVModuleWidget::createContextMenu() {
	Menu *menu = gScene->createMenu();

	// MenuLabel *menuLabel = new MenuLabel();
	// menuLabel->text = model->author + " " + model->name + " " + model->plugin->version;
	// menu->addChild(menuLabel);

	// ResetMenuItem *resetItem = new ResetMenuItem();
	// resetItem->text = "Initialize";
	// resetItem->rightText = WINDOW_MOD_KEY_NAME "+I";
	// resetItem->VCVModuleWidget = this;
	// menu->addChild(resetItem);

	// RandomizeMenuItem *randomizeItem = new RandomizeMenuItem();
	// randomizeItem->text = "Randomize";
	// randomizeItem->rightText = WINDOW_MOD_KEY_NAME "+R";
	// randomizeItem->VCVModuleWidget = this;
	// menu->addChild(randomizeItem);

	// DisconnectMenuItem *disconnectItem = new DisconnectMenuItem();
	// disconnectItem->text = "Disconnect cables";
	// disconnectItem->rightText = WINDOW_MOD_KEY_NAME "+U";
	// disconnectItem->VCVModuleWidget = this;
	// menu->addChild(disconnectItem);

	// CloneMenuItem *cloneItem = new CloneMenuItem();
	// cloneItem->text = "Duplicate";
	// cloneItem->rightText = WINDOW_MOD_KEY_NAME "+D";
	// cloneItem->VCVModuleWidget = this;
	// menu->addChild(cloneItem);

	// DeleteMenuItem *deleteItem = new DeleteMenuItem();
	// deleteItem->text = "Delete";
	// deleteItem->rightText = "Backspace/Delete";
	// deleteItem->VCVModuleWidget = this;
	// menu->addChild(deleteItem);

	// appendContextMenu(menu);

	return menu;
}


} // namespace rack