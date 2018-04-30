# miRack

**miRack** is a fork of [VCVRack](http://github.com/VCVRack/Rack) (an open-source virtual modular synthesizer) with optimisations and tweaks primarily targeting Raspberry Pi, ASUS Tinker Board and similar hardware. Can be used on desktop systems as well.

**Rack** itself is the main application/engine/plugin host. See below for information about plugins implementing actual modules.

miRack is a work in progress. Some features are broken or deliberately turned off. Read carefully below.

## Comparison to VCVRack

* Reworked and greatly optimised user interface rendering.

* Multithreaded audio/signal processing.

* Individual plugins are being forked and optimised if it is neccessary.

* Only stereo audio output is supported. VCVRack Bridge is not supported. **A patch will not run unless there's an Audio Out module with a valud output device.** This is likely to be improved in future but is not a high priority task at the moment.

* Engine sample rate must match the audio output sample rate (and will change automatically when changing the output sample rate). Some modules have been configure to work most efficiently when the sample rate is set to **48000 Hz**, which should be used to avoid CPU-intensive resampling.

* Some visual effects are turned off or simplified. This may change in future, but in general due to constrained CPU resources, audio processing and UI performance is the main priority.

## Platforms

Currently the development is done using ASUS Tinker Board, with Raspberry Pi 3 being the second main supported platform. There's a lot of similar ARM-based boards that should work too. On desktop, the development is done on macOS, and I guess Linux should be supported as well but it's not being tested.

OpenGL ES will be used on ARM boards for for rendering, OpenGL will be used on desktop.

### Performance

ARM-based hardware is not able to run complex patches that are possible on desktop. Some modules may perform better and some worse. Only a few of the plugins have been checked for possible optimisations and optimised so far. See below and ensure to install an optimised version if it's available.

Currently, Rack will spawn 3 audio processing threads (because Raspberry Pi and Tinker Board have 4 CPU cores) and will attempt to assing higher priority to them (requires the user to have appropriate permissions). More details on configuring system for better performance and to ensure uninterrupted audio will be added later.

Please note that Tinker Board that I'm currently using for development is about twice faster than Raspberry Pi 3. Also, make sure your board has proper active cooling. Without it, all these boards get quite hot under heavy load and that will cause CPU frequency to reduce very quickly.

### Tinker Board Notes

When configuring Audio Out module, choose "hw:rockchip,miniarm-codec,0" for HDMI output, and "hw:USB Audio OnBoard,2" for 3.5mm jack output.

Due to some issues with Xorg, the framerate is less than the hardware can achive. Support for running without an X server is in development.

### Raspberry Pi Notes

Raspberry Pi 3 is supported. I don't expect any good results with older models.

GL driver must be enabled either manually via `config.txt` or using `raspi-config` (Advanced Options -> GL Driver).

Right click the volume icon on the desktop taskbar to switch between HDMI and 3.5mm jack audio output.

## Plugins

VCVRack plugins are source-compatible with miRack. There may be issues with individual plugins, but the ultimate goal is to maintain source code compatibility. However plugins are not binary-compatible, so closed-source plugins are not supported. This is unlikely to change in the future, although existing binaries would not work on ARM-based boards anyway.

Some plugins have been forked to include optimisations and tweaks. See (VCVRack plugin list](https://vcvrack.com/plugins.html) and Installing Plugins section below.

## Building & Running

When cloning this repository, use `--recursive` option, or do `git submodule update --init --recursive`.

Assuming you have the basic tools (GCC and make on Linux, Xcode and [Homebrew](http://brew.sh) on macOS), execute `make prereq`. This will attempt to automatically install the required packages on Debian-based distros (Tinker OS and Raspbian) and macOS.

Build dependencies with `make dep`, and then build Rack with `make`.

Install Fundamental module pack with `make +Fundamental` to have several basic modules to begin with.

To run Rack, execute `make run`.

## Installing Plugins

### Automatic

There's a list of plugins that can be viewed by executing `make list-plugins`. To install plugins, execute `make +PluginSlug +PluginSlug ...`. You will get the slugs from the list, and also they are shown by Rack when you try to open a patch that uses modules from missing plugins. Also, you can install all plugins at once with `make +all`.

Again, most of the plugins have not been tested and may or may not be usable on ARM hardware in their current states (and some may fail to build).

Installing plugins this way ensures that an optimised version will be installed if it's available.

Also, since miRack is under active development, plugins need to be rebuilt when miRack is updated. This can be done for all installed plugins at once by executing `make allplugins`.

### Manual

Clone a plugin respository into a folder inside `plugins` folder. Use `--recursive` option, or do `git submodule update --init --recursive` afterwards.

To build, execute `make -C plugins/PLUGIN_FOLDER`. No installation is required.

## Licenses

All **source code** in this repository is licensed under [BSD-3-Clause](LICENSE.txt).

**Component Library graphics** in `res/ComponentLibrary` are licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) by [Grayscale](http://grayscale.info/). Commercial plugins must request a commercial license to use Component Library graphics by emailing contact@vcvrack.com.
