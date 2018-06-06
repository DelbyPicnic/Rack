#include "audio.hpp"
#include "util/common.hpp"
#include "bridge.hpp"
#include "engine.hpp"
#include <emscripten.h>


namespace rack {


AudioIO::AudioIO() {
#ifndef ARCH_WEB
	setDriver(RtAudio::UNSPECIFIED);
#endif
}

AudioIO::~AudioIO() {
	closeStream();
}

std::vector<int> AudioIO::getDrivers() {
	std::vector<int> drivers;
#ifndef ARCH_WEB
	std::vector<RtAudio::Api> apis;
	RtAudio::getCompiledApi(apis);
	for (RtAudio::Api api : apis)
		drivers.push_back((int) api);
	// Add fake Bridge driver
	// drivers.push_back(BRIDGE_DRIVER);
#endif
	return drivers;
}

std::string AudioIO::getDriverName(int driver) {
#ifndef ARCH_WEB
	switch (driver) {
		case RtAudio::UNSPECIFIED: return "Unspecified";
		case RtAudio::LINUX_ALSA: return "ALSA";
		case RtAudio::LINUX_PULSE: return "PulseAudio";
		case RtAudio::LINUX_OSS: return "OSS";
		case RtAudio::UNIX_JACK: return "JACK";
		case RtAudio::MACOSX_CORE: return "Core Audio";
		case RtAudio::WINDOWS_WASAPI: return "WASAPI";
		case RtAudio::WINDOWS_ASIO: return "ASIO";
		case RtAudio::WINDOWS_DS: return "DirectSound";
		case RtAudio::RTAUDIO_DUMMY: return "Dummy Audio";
		case BRIDGE_DRIVER: return "Bridge";
		default: return "Unknown";
	}
#else
	return "Web Audio";
#endif
}

void AudioIO::setDriver(int driver) {
#ifndef ARCH_WEB
	// Close device
	setDevice(-1, 0);

	// Close driver
	if (rtAudio) {
		delete rtAudio;
		rtAudio = NULL;
	}
	this->driver = 0;

	// Open driver
	if (driver >= 0) {
		rtAudio = new RtAudio((RtAudio::Api) driver);
		this->driver = (int) rtAudio->getCurrentApi();
	}
	else if (driver == BRIDGE_DRIVER) {
		this->driver = BRIDGE_DRIVER;
	}
#endif
}

int AudioIO::getDeviceCount() {
#ifndef ARCH_WEB
	if (rtAudio) {
		return rtAudio->getDeviceCount();
	}
	else if (driver == BRIDGE_DRIVER) {
		return BRIDGE_NUM_PORTS;
	}
	return 0;
#else
	return 1;
#endif
}

#ifndef ARCH_WEB
bool AudioIO::getDeviceInfo(int device, RtAudio::DeviceInfo *deviceInfo) {
	if (!deviceInfo)
		return false;

	if (rtAudio) {
		if (device == this->device) {
			*deviceInfo = this->deviceInfo;
			return true;
		}
		else {
			try {
				*deviceInfo = rtAudio->getDeviceInfo(device);
				return true;
			}
			catch (RtAudioError &e) {
				warn("Failed to query RtAudio device: %s", e.what());
			}
		}
	}

	return false;
}
#endif

int AudioIO::getDeviceChannels(int device) {
#ifndef ARCH_WEB
	if (device < 0)
		return 0;

	if (rtAudio) {
		RtAudio::DeviceInfo deviceInfo;
		if (getDeviceInfo(device, &deviceInfo))
			return max((int) deviceInfo.inputChannels, (int) deviceInfo.outputChannels);
	}
	else if (driver == BRIDGE_DRIVER) {
		return max(BRIDGE_OUTPUTS, BRIDGE_INPUTS);
	}
	return 0;
#else
	return 2;
#endif
}

std::string AudioIO::getDeviceName(int device) {
	if (device < 0)
		return "";

#ifndef ARCH_WEB
	if (rtAudio) {
		RtAudio::DeviceInfo deviceInfo;
		if (getDeviceInfo(device, &deviceInfo))
			return deviceInfo.name;
	}
	else if (driver == BRIDGE_DRIVER) {
		return stringf("%d", device + 1);
	}
	return "";
#else
	return "Default";
#endif
}

std::string AudioIO::getDeviceDetail(int device, int offset) {
	if (device < 0)
		return "";

#ifndef ARCH_WEB
	if (rtAudio) {
		RtAudio::DeviceInfo deviceInfo;
		if (getDeviceInfo(device, &deviceInfo)) {
			std::string deviceDetail = stringf("%s (", deviceInfo.name.c_str());
			if (offset < (int) deviceInfo.inputChannels)
				deviceDetail += stringf("%d-%d in", offset + 1, min(offset + maxChannels, (int) deviceInfo.inputChannels));
			if (offset < (int) deviceInfo.inputChannels && offset < (int) deviceInfo.outputChannels)
				deviceDetail += ", ";
			if (offset < (int) deviceInfo.outputChannels)
				deviceDetail += stringf("%d-%d out", offset + 1, min(offset + maxChannels, (int) deviceInfo.outputChannels));
			deviceDetail += ")";
			return deviceDetail;
		}
	}
	else if (driver == BRIDGE_DRIVER) {
		return stringf("Port %d", device + 1);
	}
	return "";
#else
	return "Default";
#endif
}

void AudioIO::setDevice(int device, int offset) {
	closeStream();
	this->device = device;
	this->offset = offset;
	openStream();
	onDeviceChange();
}

std::vector<int> AudioIO::getSampleRates() {
#ifndef ARCH_WEB
	if (rtAudio) {
		try {
			RtAudio::DeviceInfo deviceInfo = rtAudio->getDeviceInfo(device);
			std::vector<int> sampleRates(deviceInfo.sampleRates.begin(), deviceInfo.sampleRates.end());
			return sampleRates;
		}
		catch (RtAudioError &e) {
			warn("Failed to query RtAudio device: %s", e.what());
		}
	}
#endif

	return {};
}

void AudioIO::setSampleRate(int sampleRate) {
	if (sampleRate == this->sampleRate)
		return;
	closeStream();
	this->sampleRate = sampleRate;
	openStream();
	engineSetSampleRate(sampleRate);
}

std::vector<int> AudioIO::getBlockSizes() {
#ifndef ARCH_WEB
	if (rtAudio) {
		return {/*64,*/ 128, 256, 512, 1024, 2048, 4096};
	}
#else
	return {/*64,*/ 512, 1024, 2048, 4096};
#endif
	return {};
}

void AudioIO::setBlockSize(int blockSize) {
	//TODO: UI is not updated accordingly in this case
	blockSize = max(blockSize, getBlockSizes()[0]);
	if (blockSize == this->blockSize)
		return;
	closeStream();
	this->blockSize = blockSize;
	openStream();
}

void AudioIO::setChannels(int numOutputs, int numInputs) {
	this->numOutputs = numOutputs;
	this->numInputs = numInputs;
	onChannelsChange();
}


#ifndef ARCH_WEB
static int rtCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData) {
	AudioIO *audioIO = (AudioIO*) userData;
	assert(audioIO);
	audioIO->processStream((const float *) inputBuffer, (float *) outputBuffer, nFrames);
	return 0;
}
#endif

float buf[44100*10*2];

extern "C" void processAudioJS(void *self) {
	AudioIO *audio = (AudioIO*)self;

	audio->processAudio();	
}

void AudioIO::processAudio() {
	processStream(NULL, buf, blockSize);
}

void AudioIO::openStream() {
	if (device < 0)
		return;

#ifndef ARCH_WEB
	if (rtAudio) {
		// Open new device
		try {
			deviceInfo = rtAudio->getDeviceInfo(device);
		}
		catch (RtAudioError &e) {
			warn("Failed to query RtAudio device: %s", e.what());
			return;
		}

		if (rtAudio->isStreamOpen())
			return;

		//setChannels(clamp((int) deviceInfo.outputChannels - offset, 0, maxChannels), clamp((int) deviceInfo.inputChannels - offset, 0, maxChannels));
		setChannels(min(deviceInfo.outputChannels, 2), 0);

		if (numOutputs == 0 && numInputs == 0) {
			warn("RtAudio device %d has 0 inputs and 0 outputs", device);
			return;
		}

		RtAudio::StreamParameters outParameters;
		outParameters.deviceId = device;
		outParameters.nChannels = numOutputs;
		outParameters.firstChannel = offset;

		RtAudio::StreamParameters inParameters;
		inParameters.deviceId = device;
		inParameters.nChannels = numInputs;
		inParameters.firstChannel = offset;

		RtAudio::StreamOptions options;
		options.flags |= RTAUDIO_JACK_DONT_CONNECT;
		// Without this, ALSA defaults to 4 periods (buffers) which is too much
		if (driver == RtAudio::LINUX_ALSA)
			options.flags |= RTAUDIO_MINIMIZE_LATENCY;

		int closestSampleRate = deviceInfo.preferredSampleRate;
		for (int sr : deviceInfo.sampleRates) {
			if (abs(sr - sampleRate) < abs(closestSampleRate - sampleRate)) {
				closestSampleRate = sr;
			}
		}

		try {
			info("Opening audio RtAudio device %d with %d in %d out", device, numInputs, numOutputs);
			rtAudio->openStream(
				numOutputs == 0 ? NULL : &outParameters,
				numInputs == 0 ? NULL : &inParameters,
				RTAUDIO_FLOAT32, closestSampleRate, (unsigned int*) &blockSize,
				&rtCallback, this, &options, NULL);
		}
		catch (RtAudioError &e) {
			warn("Failed to open RtAudio stream: %s", e.what());
			return;
		}

		try {
			info("Starting RtAudio stream %d", device);
			rtAudio->startStream();
		}
		catch (RtAudioError &e) {
			warn("Failed to start RtAudio stream: %s", e.what());
			return;
		}

		// Update sample rate because this may have changed
		this->sampleRate = rtAudio->getStreamSampleRate();
		engineSetSampleRate(sampleRate);
		onOpenStream();
	} else if (driver == BRIDGE_DRIVER) {
		setChannels(BRIDGE_OUTPUTS, BRIDGE_INPUTS);
		bridgeAudioSubscribe(device, this);
	}
#else
	sampleRate = EM_ASM_INT({
		if (!Module.sourceNode) {
			Module.context = new(window.AudioContext || window.webkitAudioContext)();
			Module.audioBuf = $1;
			Module.sourceNode = Module.context.createOscillator();
			Module.sourceNode.start(0);
		}

		Module.audioNode = Module.context.createScriptProcessor($2, 0, 2);
		Module.audioNode.onaudioprocess = function(ev) {
			ccall('processAudioJS', 'v', ['number'], [$0]);

			var channel0 = ev.outputBuffer.getChannelData(0);
			var channel1 = ev.outputBuffer.getChannelData(1);
			var pData = Module.audioBuf;
			pData >>= 2;
			for (var i = 0; i < $2; ++i) {
				channel0[i] = HEAPF32[pData++];
				channel1[i] = HEAPF32[pData++];
			}
		};
		Module.sourceNode.connect(Module.audioNode);
		Module.audioNode.connect(Module.context.destination);

		return Module.context.sampleRate;
	}, this, buf, blockSize);
	
	engineSetSampleRate(sampleRate);
#endif
}

void AudioIO::closeStream() {
	setChannels(0, 0);

#ifndef ARCH_WEB
	if (rtAudio) {
		if (rtAudio->isStreamRunning()) {
			info("Stopping RtAudio stream %d", device);
			try {
				rtAudio->stopStream();
			}
			catch (RtAudioError &e) {
				warn("Failed to stop RtAudio stream %s", e.what());
			}
		}
		if (rtAudio->isStreamOpen()) {
			info("Closing RtAudio stream %d", device);
			try {
				rtAudio->closeStream();
			}
			catch (RtAudioError &e) {
				warn("Failed to close RtAudio stream %s", e.what());
			}
		}
		deviceInfo = RtAudio::DeviceInfo();
	}
	else if (driver == BRIDGE_DRIVER) {
		bridgeAudioUnsubscribe(device, this);
	}
#else
	EM_ASM({
		if (Module.audioNode) {
			Module.sourceNode.disconnect();
			Module.audioNode.disconnect();
			Module.audioNode.onaudioprocess = null;
			Module.audioNode = null;
		}
	});
#endif

	onCloseStream();
}

json_t *AudioIO::toJson() {
	json_t *rootJ = json_object();
	json_object_set_new(rootJ, "driver", json_integer(driver));
	std::string deviceName = getDeviceName(device);
	json_object_set_new(rootJ, "deviceName", json_string(deviceName.c_str()));
	json_object_set_new(rootJ, "offset", json_integer(offset));
	json_object_set_new(rootJ, "maxChannels", json_integer(maxChannels));
	json_object_set_new(rootJ, "sampleRate", json_integer(sampleRate));
	json_object_set_new(rootJ, "blockSize", json_integer(blockSize));
	return rootJ;
}

void AudioIO::fromJson(json_t *rootJ) {
	closeStream();

	json_t *driverJ = json_object_get(rootJ, "driver");
	if (driverJ)
		setDriver(json_number_value(driverJ));

	json_t *deviceNameJ = json_object_get(rootJ, "deviceName");
	if (deviceNameJ) {
		std::string deviceName = json_string_value(deviceNameJ);
		// Search for device ID with equal name
		for (int device = 0; device < getDeviceCount(); device++) {
			if (getDeviceName(device) == deviceName) {
				this->device = device;
				break;
			}
		}
	}

	json_t *offsetJ = json_object_get(rootJ, "offset");
	if (offsetJ)
		offset = json_integer_value(offsetJ);

	json_t *maxChannelsJ = json_object_get(rootJ, "maxChannels");
	if (maxChannelsJ)
		maxChannels = json_integer_value(maxChannelsJ);

	json_t *sampleRateJ = json_object_get(rootJ, "sampleRate");
	if (sampleRateJ)
		sampleRate = json_integer_value(sampleRateJ);

	json_t *blockSizeJ = json_object_get(rootJ, "blockSize");
	if (blockSizeJ)
		blockSize = json_integer_value(blockSizeJ);

	openStream();
	onDeviceChange();
}


} // namespace rack
