#include "app.hpp"
#include "util/color.hpp"


namespace rack {


void MultiLightWidget::addBaseColor(NVGcolor baseColor) {
	baseColors.push_back(baseColor);
}

void MultiLightWidget::setValues(const std::vector<float> &values) {
	assert(values.size() == baseColors.size());
	NVGcolor old = color;
	color = nvgRGBAf(0, 0, 0, 0);
	for (size_t i = 0; i < baseColors.size(); i++) {
		NVGcolor c = baseColors[i];
		c.a *= clamp(values[i], 0.f, 1.f);
		color = colorScreen(color, c);
	}
	color = colorClip(color);
	if (color.a != old.a || color.r != old.r || color.g != old.g || color.b != old.b)
		needsRender = true;
}


} // namespace rack
