#include "ui.hpp"


namespace mirack {

void ChoiceButton::draw(NVGcontext *vg) {
	bndChoiceButton(vg, 0.0, 0.0, box.size.x, box.size.y, BND_CORNER_NONE, state, -1, text.c_str());
}


} // namespace mirack
