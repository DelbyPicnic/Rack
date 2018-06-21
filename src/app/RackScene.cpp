#include "app.hpp"
#include "window.hpp"
#include "util/request.hpp"
#include <string.h>
#include <thread>


namespace rack {

struct CalibrationWidget : OpaqueWidget {
	std::vector<Vec> pts;
	std::vector<Vec> touchpts;

	void draw(NVGcontext *vg) override {
		nvgSave(vg);

		nvgBeginPath(vg);
		nvgAllowMergeSubpaths(vg);

		for (auto p : pts)
			nvgRect(vg, p.x-5, p.y-5, 10, 10);

		nvgFillColor(vg, nvgRGBf(1,0,0));
		nvgFill(vg);


		nvgBeginPath(vg);
		nvgAllowMergeSubpaths(vg);

		for (auto p : touchpts)
			nvgRect(vg, p.x-5, p.y-5, 10, 10);

		nvgFillColor(vg, nvgRGBf(0,1,0));
		nvgFill(vg);

		nvgRestore(vg);
	};

	void onMouseDown(EventMouseDown &e) override {
		touchpts.push_back(e.pos);
		e.consumed = true;
	};

	void onResize() override {
		pts.resize(0);
		touchpts.resize(0);

		int firstx = 16, lastx = box.size.x - 16;
		int firsty = 16, lasty = box.size.y - 16;
		int dx = box.size.x / 5;
		int dy = box.size.y / 5;

		for (int x = firstx; x < box.size.x; x+=dx) {
			for (int y = firstx; y < box.size.y; y+=dy)
				pts.push_back(Vec(x, y));
			pts.push_back(Vec(x, lasty));
		}
		for (int y = firstx; y < box.size.y; y+=dy)
			pts.push_back(Vec(lastx, y));
		pts.push_back(Vec(lastx, lasty));
	};
};
CalibrationWidget *cw;


RackScene::RackScene() {
	scrollWidget = new RackScrollWidget();
	{
		zoomWidget = new ZoomWidget();
		{
			assert(!gRackWidget);
			gRackWidget = new RackWidget();
			zoomWidget->addChild(gRackWidget);
		}
		scrollWidget->container->addChild(zoomWidget);
	}
	addChild(scrollWidget);

	gToolbar = new Toolbar();
	addChild(gToolbar);
	scrollWidget->box.pos.y = gToolbar->box.size.y;

	cw = new CalibrationWidget();
	// addChild(cw);
}

void RackScene::onResize() {
	// Resize owned descendants
	gToolbar->box.size.x = box.size.x;
	scrollWidget->box.size = box.size.minus(scrollWidget->box.pos);
	cw->box = box;

	Scene::onResize();
}

void RackScene::draw(NVGcontext *vg) {
	Scene::draw(vg);
}

void RackScene::onHoverKey(EventHoverKey &e) {
	Widget::onHoverKey(e);

	if (!e.consumed) {
		switch (e.key) {
			case GLFW_KEY_N: {
				if (windowIsModPressed() && !windowIsShiftPressed()) {
					gRackWidget->reset();
					e.consumed = true;
				}
			} break;
			case GLFW_KEY_Q: {
				if (windowIsModPressed() && !windowIsShiftPressed()) {
					windowClose();
					e.consumed = true;
				}
			} break;
			case GLFW_KEY_O: {
				if (windowIsModPressed() && !windowIsShiftPressed()) {
					gRackWidget->openDialog();
					e.consumed = true;
				}
				if (windowIsModPressed() && windowIsShiftPressed()) {
					gRackWidget->revert();
					e.consumed = true;
				}
			} break;
			case GLFW_KEY_S: {
				if (windowIsModPressed() && !windowIsShiftPressed()) {
					gRackWidget->saveDialog();
					e.consumed = true;
				}
				if (windowIsModPressed() && windowIsShiftPressed()) {
					gRackWidget->saveAsDialog();
					e.consumed = true;
				}
			} break;
			case GLFW_KEY_ENTER: {
				appModuleBrowserCreate();
				e.consumed = true;
			} break;
		}
	}
}

void RackScene::onPathDrop(EventPathDrop &e) {
	if (e.paths.size() >= 1) {
		const std::string& firstPath = e.paths.front();
		if (stringExtension(firstPath) == "vcv") {
			gRackWidget->loadPatch(firstPath);
			e.consumed = true;
		}
	}

	if (!e.consumed)
		Scene::onPathDrop(e);
}


} // namespace rack
