#include "app.hpp"
#include "engine.hpp"
#include "plugin.hpp"
#include "window.hpp"
#include "settings.hpp"
#include "asset.hpp"
#include <map>
#include <algorithm>
#include "osdialog.h"


namespace rack {


struct ModuleContainer : Widget {
};


RackWidget::RackWidget() {
	moduleContainer = new ModuleContainer();
	wireContainer = new WireContainer();
}

RackWidget::~RackWidget() {
}

void RackWidget::clear() {
	// wireContainer->activeWire = NULL;
	// wireContainer->clearChildren();
	// moduleContainer->clearChildren();

	// gRackScene->scrollWidget->offset = Vec(0, 0);
}

void RackWidget::reset() {
	// if (osdialog_message(OSDIALOG_INFO, OSDIALOG_OK_CANCEL, "Clear patch and start over?")) {
	// 	clear();
	// 	// Fails silently if file does not exist
	// 	loadPatch(assetLocal("template.vcv"));
	// 	lastPath = "";
	// }
}

void RackWidget::openDialog() {
}

void RackWidget::saveDialog() {
}

void RackWidget::saveAsDialog() {
}

void RackWidget::savePatch(std::string path) {
}

void RackWidget::loadPatch(std::string path) {
}

void RackWidget::revert() {
}

void RackWidget::disconnect() {
}

json_t *RackWidget::toJson() {
	// root
	json_t *rootJ = json_object();

	return rootJ;
}

void RackWidget::fromJson(json_t *rootJ) {
}

void RackWidget::addModule(ModuleWidget *m) {
	// moduleContainer->addChild(m);
	// m->create();
}

void RackWidget::deleteModule(ModuleWidget *m) {
	// m->_delete();
	// moduleContainer->removeChild(m);
}

void RackWidget::cloneModule(ModuleWidget *m) {
	// // Create new module from model
	// ModuleWidget *clonedModuleWidget = m->model->createModuleWidget();
	// // JSON serialization is the most straightforward way to do this
	// json_t *moduleJ = m->toJson();
	// clonedModuleWidget->fromJson(moduleJ);
	// json_decref(moduleJ);
	// Rect clonedBox = clonedModuleWidget->box;
	// clonedBox.pos = m->box.pos;
	// requestModuleBoxNearest(clonedModuleWidget, clonedBox);
	// addModule(clonedModuleWidget);
}

bool RackWidget::requestModuleBox(ModuleWidget *m, Rect box) {
	// if (box.pos.x < 0 || box.pos.y < 0)
	// 	return false;

	// for (Widget *child2 : moduleContainer->children) {
	// 	if (m == child2) continue;
	// 	if (box.intersects(child2->box)) {
	// 		return false;
	// 	}
	// }
	// m->box = box;
	return true;
}

bool RackWidget::requestModuleBoxNearest(ModuleWidget *m, Rect box) {
	// // Create possible positions
	// int x0 = roundf(box.pos.x / RACK_GRID_WIDTH);
	// int y0 = roundf(box.pos.y / RACK_GRID_HEIGHT);
	// std::vector<Vec> positions;
	// for (int y = max(0, y0 - 8); y < y0 + 8; y++) {
	// 	for (int x = max(0, x0 - 400); x < x0 + 400; x++) {
	// 		positions.push_back(Vec(x * RACK_GRID_WIDTH, y * RACK_GRID_HEIGHT));
	// 	}
	// }

	// // Sort possible positions by distance to the requested position
	// std::sort(positions.begin(), positions.end(), [box](Vec a, Vec b) {
	// 	return a.minus(box.pos).norm() < b.minus(box.pos).norm();
	// });

	// // Find a position that does not collide
	// for (Vec position : positions) {
	// 	Rect newBox = box;
	// 	newBox.pos = position;
	// 	if (requestModuleBox(m, newBox))
	// 		return true;
	// }
	return false;
}

void RackWidget::step() {
}

void RackWidget::draw(NVGcontext *vg) {
}

void RackWidget::onMouseMove(EventMouseMove &e) {
}

void RackWidget::onMouseDown(EventMouseDown &e) {
}

void RackWidget::onZoom(EventZoom &e) {
}


} // namespace rack
