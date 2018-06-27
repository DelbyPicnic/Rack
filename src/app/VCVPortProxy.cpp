#include "app.hpp"
#include "window.hpp"
#include "engine.hpp"
#include "componentlibrary.hpp"

#include "compat/include/app.hpp"

namespace mirack {

VCVPortProxy::VCVPortProxy(rack::Port *target) {
	this->target = target;
}

VCVPortProxy::~VCVPortProxy() {
	// // plugLight is not a child and is thus owned by the Port, so we need to delete it here
	// delete plugLight;
	// gRackWidget->wireContainer->removeAllWires(this);
}

void VCVPortProxy::step() {
	// std::vector<float> values(2);
	// if (type == INPUT) {
	// 	values[0] = module->inputs[portId].plugLights[0].getBrightness();
	// 	values[1] = module->inputs[portId].plugLights[1].getBrightness();
	// }
	// else {
	// 	values[0] = module->outputs[portId].plugLights[0].getBrightness();
	// 	values[1] = module->outputs[portId].plugLights[1].getBrightness();
	// }
	// plugLight->setValues(values);
}

void VCVPortProxy::draw(NVGcontext *vg) {
	target->draw(vg);
	// WireWidget *activeWire = gRackWidget->wireContainer->activeWire;
	// if (activeWire) {
	// 	// Dim the Port if the active wire cannot plug into this Port
	// 	if (type == INPUT ? activeWire->inputPort : activeWire->outputPort)
	// 		nvgGlobalAlpha(vg, 0.5);
	// }
}

void VCVPortProxy::onMouseDown(EventMouseDown &e) {
	if (e.button == 1) {
		gRackWidget->wireContainer->removeTopWire(this);

		// HACK
		// Update hovered*Port of active wire if applicable
		EventDragEnter e;
		onDragEnter(e);
	}
	e.consumed = true;
	e.target = this;
}

void VCVPortProxy::onDragStart(EventDragStart &e) {
	// Try to grab wire on top of stack
	WireWidget *wire = gRackWidget->wireContainer->getTopWire(this);
	if (type == OUTPUT && (windowIsModPressed() || windowIsShiftPressed())) {
		wire = NULL;
	}

	if (wire) {
		// Disconnect existing wire
		if (type == INPUT)
			wire->inputPort = NULL;
		else
			wire->outputPort = NULL;
		wire->updateWire();
	}
	else {
		// Create a new wire
		wire = new WireWidget();
		if (type == INPUT)
			wire->inputPort = this;
		else
			wire->outputPort = this;
	}
	gRackWidget->wireContainer->setActiveWire(wire);
}

void VCVPortProxy::onDragEnd(EventDragEnd &e) {
	// FIXME
	// If the source Port is deleted, this will be called, removing the cable
	gRackWidget->wireContainer->commitActiveWire();
}

void VCVPortProxy::onDragDrop(EventDragDrop &e) {
}

void VCVPortProxy::onDragEnter(EventDragEnter &e) {
	// Reject ports if this is an input port and something is already plugged into it
	if (type == INPUT) {
		WireWidget *topWire = gRackWidget->wireContainer->getTopWire(this);
		if (topWire)
			return;
	}

	WireWidget *activeWire = gRackWidget->wireContainer->activeWire;
	if (activeWire) {
		if (type == INPUT)
			activeWire->hoveredInputPort = this;
		else
			activeWire->hoveredOutputPort = this;
	}
}

void VCVPortProxy::onDragLeave(EventDragEnter &e) {
	WireWidget *activeWire = gRackWidget->wireContainer->activeWire;
	if (activeWire) {
		if (type == INPUT)
			activeWire->hoveredInputPort = NULL;
		else
			activeWire->hoveredOutputPort = NULL;
	}
}


} // namespace mirack
