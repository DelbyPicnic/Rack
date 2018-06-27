#include "app.hpp"
#include "window.hpp"


namespace mirack {


void RackScrollWidget::step() {
	Vec pos = gMousePos;
	Rect viewport = getViewport(box.zeroPos());
	// Scroll rack if dragging cable near the edge of the screen
	if (gRackWidget->wireContainer->activeWire) {
		float margin = 20.0;
		float speed = 15.0;
		
		if (pos.x <= viewport.pos.x + margin)
			offset.x -= speed;
		else if (pos.x >= viewport.pos.x + viewport.size.x - margin)
			offset.x += speed;
		else if (pos.y <= viewport.pos.y + margin)
			offset.y -= speed;
		else if (pos.y >= viewport.pos.y + viewport.size.y - margin)
			offset.y += speed;
		else 
			goto skip;

		updateForOffsetChange();
		skip:;
	}
	
	ScrollWidget::step();
}

void RackScrollWidget::updateForOffsetChange() {
	// Resize to be a bit larger than the ScrollWidget viewport
	gRackWidget->box.size = box.size
		.minus(container->box.pos)
		.plus(Vec(500, 500))
		.div(gRackScene->zoomWidget->zoom);

	gRackScene->zoomWidget->box.size = gRackWidget->box.size.mult(gRackScene->zoomWidget->zoom);

	ScrollWidget::updateForOffsetChange();
}

} // namespace mirack
