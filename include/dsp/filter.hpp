#pragma once

#include "util/math.hpp"


namespace rack {

struct RCFilter {
	float c = 0.f, c1, c2;
	float xstate = 0;
	float ystate = 0;

	// `r` is the ratio between the cutoff frequency and sample rate, i.e. r = f_c / f_s
	void setCutoff(float r) {
		c = 2.f / r;
		c1 = 1.0f - c;
		c2 = 1.0f / (1.0f + c);
	}
	void process(float x) {
		float y = (x + xstate - ystate * c1) * c2;
		xstate = x;
		ystate = y;
	}
	float lowpass() {
		return ystate;
	}
	float highpass() {
		return xstate - ystate;
	}
};


struct PeakFilter {
	float state = 0.f;
	float c = 0.f;

	/** Rate is lambda / sampleRate */
	void setRate(float r) {
		c = 1.f - r;
	}
	void process(float x) {
		if (x > state)
			state = x;
		state *= c;
	}
	float peak() {
		return state;
	}
};


struct SlewLimiter {
	float rise = 1.f;
	float fall = 1.f;
	float out = 0.f;

	void setRiseFall(float rise, float fall) {
		this->rise = rise;
		this->fall = fall;
	}
	float process(float in) {
		out = clamp(in, out - fall, out + rise);
		return out;
	}
};


/** Applies exponential smoothing to a signal with the ODE
dy/dt = x * lambda
*/
struct ExponentialFilter {
	float out = 0.f;
	float lambda = 1.f;

	float process(float in) {
		float y = out + (in - out) * lambda;
		// If no change was detected, assume float granularity is too small and snap output to input
		if (out == y)
			out = in;
		else
			out = y;
		return out;
	}
};


} // namespace rack
