#include "audio.hpp"
#include "util/common.hpp"
#include "bridge.hpp"
#include "engine.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include "tinythread.h"

#define OPENAL_DRIVER -999

ALCdevice *alDevice;
ALCcontext *alContext = NULL;
ALuint alSource;
ALuint alBuffers[10];

namespace rack {


AudioIO::AudioIO() {
	setDriver(OPENAL_DRIVER);
}

AudioIO::~AudioIO() {
	closeStream();
}

std::vector<int> AudioIO::getDrivers() {
	// std::vector<RtAudio::Api> apis;
	// RtAudio::getCompiledApi(apis);
	std::vector<int> drivers;
	// for (RtAudio::Api api : apis)
	// 	drivers.push_back((int) api);
	drivers.push_back(OPENAL_DRIVER);
	// Add fake Bridge driver
	// drivers.push_back(BRIDGE_DRIVER);
	return drivers;
}

std::string AudioIO::getDriverName(int driver) {
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
		case OPENAL_DRIVER: return "OpenAL";
		default: return "Unknown";
	}
}

void AudioIO::setDriver(int driver) {
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
	else if (driver == OPENAL_DRIVER) {
		this->driver = OPENAL_DRIVER;
	}
	else if (driver == BRIDGE_DRIVER) {
		this->driver = BRIDGE_DRIVER;
	}
}

int AudioIO::getDeviceCount() {
	if (rtAudio) {
		return rtAudio->getDeviceCount();
	}
	else if (driver == OPENAL_DRIVER) {
		return 1;
	}
	else if (driver == BRIDGE_DRIVER) {
		return BRIDGE_NUM_PORTS;
	}
	return 0;
}

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

int AudioIO::getDeviceChannels(int device) {
	if (device < 0)
		return 0;

	if (rtAudio) {
		RtAudio::DeviceInfo deviceInfo;
		if (getDeviceInfo(device, &deviceInfo))
			return max((int) deviceInfo.inputChannels, (int) deviceInfo.outputChannels);
	}
	else if (driver == OPENAL_DRIVER) {
		return 2;
	}
	else if (driver == BRIDGE_DRIVER) {
		return max(BRIDGE_OUTPUTS, BRIDGE_INPUTS);
	}
	return 0;
}

std::string AudioIO::getDeviceName(int device) {
	if (device < 0)
		return "";

	if (rtAudio) {
		RtAudio::DeviceInfo deviceInfo;
		if (getDeviceInfo(device, &deviceInfo))
			return deviceInfo.name;
	}
	else if (driver == OPENAL_DRIVER) {
		return "Default";
	}
	else if (driver == BRIDGE_DRIVER) {
		return stringf("%d", device + 1);
	}
	return "";
}

std::string AudioIO::getDeviceDetail(int device, int offset) {
	if (device < 0)
		return "";

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
	else if (driver == OPENAL_DRIVER) {
		return "Default";
	}
	else if (driver == BRIDGE_DRIVER) {
		return stringf("Port %d", device + 1);
	}
	return "";
}

void AudioIO::setDevice(int device, int offset) {
	closeStream();
	this->device = device;
	this->offset = offset;
	openStream();
	onDeviceChange();
}

std::vector<int> AudioIO::getSampleRates() {
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

	if (driver == OPENAL_DRIVER) {
		return {};
	}

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
	if (rtAudio || driver == OPENAL_DRIVER) {
		return {/*64, 128, 256, 512,*/ 1024, 2048, 4096};
	}
	return {};
}

void AudioIO::setBlockSize(int blockSize) {
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


static int rtCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData) {
	AudioIO *audioIO = (AudioIO*) userData;
	assert(audioIO);
	audioIO->processStream((const float *) inputBuffer, (float *) outputBuffer, nFrames);
	return 0;
}

float buf[44100*10*2];

static void processAudio2(void *self) {
    info("OpenAL thread started");

	AudioIO *audio = (AudioIO*)self;

	ALenum format = 0x10011;//AL_FORMAT_STEREO_FLOAT32;

	// alBufferData(alBuffers[0], format, buf, 1024*2*sizeof(float), audio->sampleRate);
	// alBufferData(alBuffers[1], format, buf, 1024*2*sizeof(float), audio->sampleRate);
 //    alSourceQueueBuffers(alSource, 2, &alBuffers[0]);
	// alSourcePlay(alSource);

    while(1) {
    	ALuint buffer = 0;
    	ALint buffersProcessed = 0;
		alGetSourcei(alSource, AL_BUFFERS_PROCESSED, &buffersProcessed);
		info("%d",buffersProcessed);
		while(buffersProcessed--) {
			alSourceUnqueueBuffers(alSource, 1, &buffer);
			audio->processStream(NULL, buf, audio->blockSize);
			alBufferData(alBuffers[0], format, buf, audio->blockSize*2*sizeof(float), audio->sampleRate);
		    alSourceQueueBuffers(alSource, 1, &buffer);
		}

		ALint state;
		alGetSourcei(alSource, AL_SOURCE_STATE, &state);
		info("state %d",state);
		if (state != AL_PLAYING)
			alSourcePlay(alSource);

		tthread::this_thread::sleep_for(tthread::chrono::milliseconds(20));
	}
}

void AudioIO::processAudio() {
	if (!alContext)
		return;

	ALenum format = 0x10011; //AL_FORMAT_STEREO_FLOAT32;

	ALuint buffer = 0;
	ALint buffersProcessed = 0;
	alGetSourcei(alSource, AL_BUFFERS_PROCESSED, &buffersProcessed);
	while(buffersProcessed--) {
		alSourceUnqueueBuffers(alSource, 1, &buffer);
		processStream(NULL, buf, blockSize);
		alBufferData(buffer, format, buf, blockSize*2*sizeof(float), sampleRate);
	    alSourceQueueBuffers(alSource, 1, &buffer);
	}

	ALint state;
	alGetSourcei(alSource, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		alSourcePlay(alSource);
}

void AudioIO::openStream() {
	if (device < 0)
		return;

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
	}
	else if (driver == OPENAL_DRIVER) {
		info("OPENAL OPENAL");
		alDevice = alcOpenDevice(NULL);
		if (!alDevice)
			fatal("Can't open OpenAL device");

		alContext = alcCreateContext(alDevice, NULL);
		if (!alcMakeContextCurrent(alContext))
			fatal("Can't init OpenAL context");

		alListener3f(AL_POSITION, 0, 0, 1.0f);
    	alListener3f(AL_VELOCITY, 0, 0, 0);

    	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
		alListenerfv(AL_ORIENTATION, listenerOri);

		alGenSources((ALuint)1, &alSource);

		alSourcef(alSource, AL_PITCH, 1);
		alSourcef(alSource, AL_GAIN, 1);
		alSource3f(alSource, AL_POSITION, 0, 0, 0);
		alSource3f(alSource, AL_VELOCITY, 0, 0, 0);
		alSourcei(alSource, AL_LOOPING, AL_FALSE);

		alGenBuffers(2, alBuffers);


		ALenum format = 0x10011;//AL_FORMAT_STEREO_FLOAT32;

		alBufferData(alBuffers[0], format, buf, blockSize*2*sizeof(float), sampleRate);
		alBufferData(alBuffers[1], format, buf, blockSize*2*sizeof(float), sampleRate);
		alBufferData(alBuffers[2], format, buf, blockSize*2*sizeof(float), sampleRate);
		alBufferData(alBuffers[3], format, buf, blockSize*2*sizeof(float), sampleRate);
	    alSourceQueueBuffers(alSource, 4, &alBuffers[0]);
		alSourcePlay(alSource);
		ALint state;
		alGetSourcei(alSource, AL_SOURCE_STATE, &state);
		info("state %d",state);

		// (new tthread::thread((void(*)(void*))processAudio2, (void*)this))->detach();
	} else if (driver == BRIDGE_DRIVER) {
		setChannels(BRIDGE_OUTPUTS, BRIDGE_INPUTS);
		bridgeAudioSubscribe(device, this);
	}
}

void AudioIO::closeStream() {
	setChannels(0, 0);

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
	else if (driver == OPENAL_DRIVER) {
	}
	else if (driver == BRIDGE_DRIVER) {
		bridgeAudioUnsubscribe(device, this);
	}

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
