#include "ui.hpp"


namespace mirack {


void PasswordField::draw(NVGcontext *vg) {
	std::string textTmp = text;
	text = std::string(textTmp.size(), '*');
	TextField::draw(vg);
	text = textTmp;
}


} // namespace mirack
