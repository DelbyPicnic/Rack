#include "midi.hpp"
#include "bridge.hpp"

namespace rack {


////////////////////
// MidiIO
////////////////////

std::vector<int> MidiIO::getDrivers() {
	std::vector<int> drivers;

#ifndef ARCH_WEB
	std::vector<RtMidi::Api> rtApis;
	RtMidi::getCompiledApi(rtApis);

	for (RtMidi::Api api : rtApis) {
		drivers.push_back((int) api);
	}
	// Add fake Bridge driver
	// drivers.push_back(BRIDGE_DRIVER);
#endif
	return drivers;
}

std::string MidiIO::getDriverName(int driver) {
#ifndef ARCH_WEB
	switch (driver) {
		case RtMidi::UNSPECIFIED: return "Unspecified";
		case RtMidi::MACOSX_CORE: return "Core MIDI";
		case RtMidi::LINUX_ALSA: return "ALSA";
		case RtMidi::UNIX_JACK: return "JACK";
		case RtMidi::WINDOWS_MM: return "Windows MIDI";
		case RtMidi::RTMIDI_DUMMY: return "Dummy MIDI";
		case BRIDGE_DRIVER: return "Bridge";
		default: return "Unknown";
	}
#else
	return EM_ASM_INT({
        return Module.midiAccess != null;
    }) ? "Web MIDI" : "Not supported";
#endif
}

int MidiIO::getDeviceCount() {
#ifndef ARCH_WEB
	if (rtMidi) {
		return rtMidi->getPortCount();
	}
	else if (driver == BRIDGE_DRIVER) {
		return BRIDGE_NUM_PORTS;
	}
	return 0;
#else
	return EM_ASM_INT({
        return Module.midiAccess ? Module.midiAccess.inputs.size : 0;
    });
#endif
}

std::string MidiIO::getDeviceName(int device) {
	if (device < 0)
		return "";

#ifndef ARCH_WEB
	if (rtMidi) {
		if (device == this->device)
			return deviceName;
		else
			return rtMidi->getPortName(device);
	}
	else if (driver == BRIDGE_DRIVER) {
		return stringf("Port %d", device + 1);
	}
	return "";
#else
	return (char*) EM_ASM_INT({
		var name = Array.from(Module.midiAccess.inputs.values())[$0].name;
		var lengthBytes = lengthBytesUTF8(name)+1; // 'jsString.length' would return the length of the string as UTF-16 units, but Emscripten C strings operate as UTF-8.
        var stringOnHeap = _malloc(lengthBytes);
        stringToUTF8(name, stringOnHeap, lengthBytes+1);
        return stringOnHeap;
    }, device);
#endif
}

std::string MidiIO::getChannelName(int channel) {
	if (channel == -1)
		return "All channels";
	else
		return stringf("Channel %d", channel + 1);
}

json_t *MidiIO::toJson() {
	json_t *rootJ = json_object();
	json_object_set_new(rootJ, "driver", json_integer(driver));
	std::string deviceName = getDeviceName(device);
	if (!deviceName.empty())
		json_object_set_new(rootJ, "deviceName", json_string(deviceName.c_str()));
	json_object_set_new(rootJ, "channel", json_integer(channel));
	return rootJ;
}

void MidiIO::fromJson(json_t *rootJ) {
	json_t *driverJ = json_object_get(rootJ, "driver");
	if (driverJ)
		setDriver(json_integer_value(driverJ));

	json_t *deviceNameJ = json_object_get(rootJ, "deviceName");
	if (deviceNameJ) {
		std::string deviceName = json_string_value(deviceNameJ);
		// Search for device with equal name
		int deviceCount = getDeviceCount();
		for (int device = 0; device < deviceCount; device++) {
			if (getDeviceName(device) == deviceName) {
				setDevice(device);
				break;
			}
		}
	}

	json_t *channelJ = json_object_get(rootJ, "channel");
	if (channelJ)
		channel = json_integer_value(channelJ);
}

////////////////////
// MidiInput
////////////////////

#ifndef ARCH_WEB
static void midiInputCallback(double timeStamp, std::vector<unsigned char> *message, void *userData) {
	if (!message) return;
	if (!userData) return;

	MidiInput *midiInput = (MidiInput*) userData;
	if (!midiInput) return;
	MidiMessage msg;
	if (message->size() >= 1)
		msg.cmd = (*message)[0];
	if (message->size() >= 2)
		msg.data1 = (*message)[1];
	if (message->size() >= 3)
		msg.data2 = (*message)[2];

	midiInput->onMessage(msg);
}
#else
extern "C" void midiInputCallbackJS(MidiInput *midiInput, uint8_t cmd, uint8_t data1, uint8_t data2) {
	// MidiInput *midiInput = (MidiInput*) userData;
	if (!midiInput) return;
	MidiMessage msg;
	msg.cmd = cmd;
	msg.data1 = data1;
	msg.data2 = data2;

	midiInput->onMessage(msg);
}
#endif

MidiInput::MidiInput() {
#ifndef ARCH_WEB
	setDriver(RtMidi::UNSPECIFIED);
#endif
}

MidiInput::~MidiInput() {
#ifndef ARCH_WEB
	setDriver(-1);
#else
	EM_ASM({
		if (Module.midiAccess)
			Array.from(Module.midiAccess.inputs.values())[$0].onmidimessage = null;
	}, this->device);	
#endif
}

void MidiInput::setDriver(int driver) {
#ifndef ARCH_WEB
	setDevice(-1);
	if (rtMidiIn) {
		delete rtMidiIn;
		rtMidi = rtMidiIn = NULL;
	}

	if (driver >= 0) {
		rtMidiIn = new RtMidiIn((RtMidi::Api) driver);
		rtMidiIn->setCallback(midiInputCallback, this);
		rtMidiIn->ignoreTypes(false, false, false);
		rtMidi = rtMidiIn;
		this->driver = rtMidiIn->getCurrentApi();
	}
	else if (driver == BRIDGE_DRIVER) {
		this->driver = BRIDGE_DRIVER;
	}
#endif
}

void MidiInput::setDevice(int device) {
#ifndef ARCH_WEB
	if (rtMidi) {
		rtMidi->closePort();

		if (device >= 0) {
			rtMidi->openPort(device);
			deviceName = rtMidi->getPortName(device);
		}
		this->device = device;
	}
	else if (driver == BRIDGE_DRIVER) {
		if (device >= 0) {
			bridgeMidiSubscribe(device, this);
		}
		else {
			bridgeMidiUnsubscribe(device, this);
		}
		this->device = device;
	}
#else
	EM_ASM({
		Array.from(Module.midiAccess.inputs.values())[$0].onmidimessage = function(msg) {
			var cmd = msg.data.length >= 1 ? msg.data[0] : 0;
			var data1 = msg.data.length >= 2 ? msg.data[1] : 0;
			var data2 = msg.data.length >= 3 ? msg.data[2] : 0;
			console.log(msg);
			ccall('midiInputCallbackJS', 'v', ['number', 'number', 'number', 'number'], [$1, cmd, data1, data2]);
		};
	}, device, this);
	this->device = device;
#endif
}

void MidiInputQueue::onMessage(MidiMessage message) {
	// Filter channel
	if (channel >= 0) {
		if (message.status() != 0xf && message.channel() != channel)
			return;
	}

	if ((int) queue.size() < queueSize)
		queue.push(message);
}

bool MidiInputQueue::shift(MidiMessage *message) {
	if (!message) return false;
	if (!queue.empty()) {
		*message = queue.front();
		queue.pop();
		return true;
	}
	return false;
}


////////////////////
// MidiOutput
////////////////////

MidiOutput::MidiOutput() {
#ifndef ARCH_WEB
	setDriver(RtMidi::UNSPECIFIED);
#endif
}

MidiOutput::~MidiOutput() {
	setDriver(-1);
}

void MidiOutput::setDriver(int driver) {
#ifndef ARCH_WEB
	setDevice(-1);
	if (rtMidiOut) {
		delete rtMidiOut;
		rtMidi = rtMidiOut = NULL;
	}

	if (driver >= 0) {
		rtMidiOut = new RtMidiOut((RtMidi::Api) driver);
		rtMidi = rtMidiOut;
		this->driver = rtMidiOut->getCurrentApi();
	}
#endif
}

void MidiOutput::setDevice(int device) {
#ifndef ARCH_WEB
	if (rtMidi) {
		rtMidi->closePort();

		if (device >= 0) {
			rtMidi->openPort(device);
			deviceName = rtMidi->getPortName(device);
		}
		this->device = device;
	}
	else if (driver == BRIDGE_DRIVER) {
	}
#endif
}


} // namespace rack
