#include "engine.hpp"
#include "compat/include/engine.hpp"

namespace mirack {

VCVModuleProxy::VCVModuleProxy(rack::Module *target) {
	this->target = target;
	inputs.resize(target->inputs.size());
	outputs.resize(target->inputs.size());

};

VCVModuleProxy::~VCVModuleProxy() {
	
};

void VCVModuleProxy::step() {
	for (int i = 0; i < target->inputs.size(); i++) {
		target->inputs[i].value = inputs[i].value;
	}
	target->step();
	for (int i = 0; i < target->outputs.size(); i++) {
		outputs[i].value = target->outputs[i].value;
	}

};

json_t *VCVModuleProxy::toJson() { 
	return target->toJson();
};

void VCVModuleProxy::fromJson(json_t *root) {
	target->fromJson(root);
};


}