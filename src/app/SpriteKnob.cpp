#include "app.hpp"


namespace mirack {

void SpriteKnob::step() {
	index = eucmod((int) roundf(rescale(value, minValue, maxValue, minIndex, maxIndex)), spriteCount);
}

} // namespace mirack
