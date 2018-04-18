#include "app.hpp"


namespace rack {


SVGScrew::SVGScrew() {
	canSquash = true;
	sw = new SVGWidget();
	addChild(sw);
}


} // namespace rack
