#include <assert.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Core.hpp"
#include "audio.hpp"
#include "dsp/samplerate.hpp"
#include "dsp/ringbuffer.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#include <RtAudio.h>
#pragma GCC diagnostic pop


#define AUDIO_OUTPUTS 0
#define AUDIO_INPUTS 2

// #define printf(a...) {}
using namespace rack;

struct AudioInterfaceIO2 : AudioIO {
	std::mutex engineMutex;
	std::condition_variable engineCv;
	std::mutex audioMutex;
	std::condition_variable audioCv;
	// Audio thread produces, engine thread consumes
	DoubleRingBuffer<Frame<AUDIO_INPUTS>, (1<<15)> inputBuffer;
	// Audio thread consumes, engine thread produces
	//DoubleRingBuffer<Frame<AUDIO_OUTPUTS>, (1<<15)> outputBuffer;
	DoubleRingBuffer<float, (1<<16)> outputBuffer;
	float buf[1<<16];
	float *bufPtr = buf;
	float *bufRead = buf;
	float *bufEnd = buf+(1<<16);
	volatile bool bufReady = false;
	bool active = false;
	Module *module;

	~AudioInterfaceIO2() {
		// Close stream here before destructing AudioInterfaceIO, so the mutexes are still valid when waiting to close.
		setDevice(-1, 0);
	}

	void processStream(const float *input, float *output, int frames) override {
		engineWaitMT();
		memcpy(output, buf, frames*2*sizeof(float));
		bufPtr = buf;
		engineStepMT(frames);
	}

	void onCloseStream() override {
		inputBuffer.clear();
		outputBuffer.clear();
		active = false;
		module->lights[0].value = 0; // Because step() won't be called
	}

	void onChannelsChange() override {
	}
};


struct AudioInterface2 : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(AUDIO_INPUT, AUDIO_INPUTS),
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(INPUT_LIGHT, AUDIO_INPUTS / 2),
		NUM_LIGHTS
	};

	AudioInterfaceIO2 audioIO;
	int lastSampleRate = 0;
	int lastNumOutputs = -1;
	int lastNumInputs = -1;

	SampleRateConverter<AUDIO_INPUTS> inputSrc;

	// in rack's sample rate
	DoubleRingBuffer<Frame<AUDIO_INPUTS>, 16> inputBuffer;
	DoubleRingBuffer<Frame<AUDIO_OUTPUTS>, 16> outputBuffer;

	AudioInterface2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		onSampleRateChange();
		audioIO.module = this;
	}

	void step() override;

	json_t *toJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "audio", audioIO.toJson());
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		json_t *audioJ = json_object_get(rootJ, "audio");
		audioIO.fromJson(audioJ);
	}

	void onReset() override {
		audioIO.setDevice(-1, 0);
	}
};


void AudioInterface2::step() {
	*(audioIO.bufPtr+0) = clamp(inputs[AUDIO_INPUT + 0].value / 10.f, -1.f, 1.f);
	*(audioIO.bufPtr+1) = clamp(inputs[AUDIO_INPUT + 1].value / 10.f, -1.f, 1.f);
	audioIO.bufPtr += 2;

	lights[INPUT_LIGHT + 0].value = (/*audioIO.active &&*/ audioIO.numOutputs > 0);
}


struct AudioInterfaceWidget2 : ModuleWidget {
	AudioInterfaceWidget2(AudioInterface2 *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetGlobal("res/Core/AudioOut.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(Port::create<PJ301MPort>(mm2px(Vec(6.5+3.7069211, 10+55.530807)), Port::INPUT, module, AudioInterface2::AUDIO_INPUT + 0));
		addInput(Port::create<PJ301MPort>(mm2px(Vec(6.5+15.307249, 10+55.530807)), Port::INPUT, module, AudioInterface2::AUDIO_INPUT + 1));
		
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mm2px(Vec(19, 62)), module, AudioInterface2::INPUT_LIGHT + 0));

		AudioWidget *audioWidget = Widget::create<AudioWidget>(mm2px(Vec(3.2122073, 14.837339)));
		audioWidget->box.size = mm2px(Vec(34.5, 28));
		audioWidget->audioIO = &module->audioIO;
		addChild(audioWidget);
	}
};


Model *modelAudioInterface2 = Model::create<AudioInterface2, AudioInterfaceWidget2>("Core", "AudioInterface", "Audio Out", EXTERNAL_TAG);
