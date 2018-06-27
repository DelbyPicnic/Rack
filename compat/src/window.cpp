#include "window.hpp"
#include "app.hpp"
#include "asset.hpp"
#include "gamepad.hpp"
#include "keyboard.hpp"
#include "util/color.hpp"

#include <map>
#include <queue>
#include <thread>

#include "osdialog.h"

// #define NANOVG_GL2_IMPLEMENTATION 1
// #define NANOVG_GL3_IMPLEMENTATION 1
// #define NANOVG_GLES2_IMPLEMENTATION 1
// #define NANOVG_GLES3_IMPLEMENTATION 1
#include "nanovg_gl.h"
// Hack to get framebuffer objects working on OpenGL 2 (we blindly assume the extension is supported)
#define NANOVG_FBO_VALID 1
#include "nanovg_gl_utils.h"
// #define BLENDISH_IMPLEMENTATION
#include "blendish.h"
// #define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "nanosvg.h"

#ifdef ARCH_MAC
	// For CGAssociateMouseAndMouseCursorPosition
	#include <ApplicationServices/ApplicationServices.h>
#endif

namespace mirack {
	void windowCursorLock();
	void windowCursorUnlock();

	bool windowIsModPressed();
	bool windowIsShiftPressed();
}


namespace rack {


GLFWwindow *gWindow = NULL;
NVGcontext *gVg = NULL;
NVGcontext *gFramebufferVg = NULL;
std::shared_ptr<Font> gGuiFont;
float gPixelRatio = 1.0;
float gWindowRatio = 1.0;
bool gAllowCursorLock = true;
int gGuiFrame;
Vec gMousePos;

void windowClose() {
	glfwSetWindowShouldClose(gWindow, GLFW_TRUE);
}

void windowCursorLock() {
	mirack::windowCursorLock();
}

void windowCursorUnlock() {
	mirack::windowCursorUnlock();
}

bool windowIsModPressed() {
	return mirack::windowIsModPressed();
}

bool windowIsShiftPressed() {
	return mirack::windowIsShiftPressed();
}

Vec windowGetWindowSize() {
	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);
	return Vec(width, height);
}

void windowSetWindowSize(Vec size) {
	int width = size.x;
	int height = size.y;
	glfwSetWindowSize(gWindow, width, height);
}

Vec windowGetWindowPos() {
	int x, y;
	glfwGetWindowPos(gWindow, &x, &y);
	return Vec(x, y);
}

void windowSetWindowPos(Vec pos) {
	int x = pos.x;
	int y = pos.y;
	glfwSetWindowPos(gWindow, x, y);
}

bool windowIsMaximized() {
	return glfwGetWindowAttrib(gWindow, GLFW_MAXIMIZED);
}

void windowSetTheme(NVGcolor bg, NVGcolor fg) {
	// Assume dark background and light foreground

	BNDwidgetTheme w;
	w.outlineColor = bg;
	w.itemColor = fg;
	w.innerColor = bg;
	w.innerSelectedColor = colorPlus(bg, nvgRGB(0x30, 0x30, 0x30));
	w.textColor = fg;
	w.textSelectedColor = fg;
	w.shadeTop = 0;
	w.shadeDown = 0;

	BNDtheme t;
	t.backgroundColor = colorPlus(bg, nvgRGB(0x30, 0x30, 0x30));
	t.regularTheme = w;
	t.toolTheme = w;
	t.radioTheme = w;
	t.textFieldTheme = w;
	t.optionTheme = w;
	t.choiceTheme = w;
	t.numberFieldTheme = w;
	t.sliderTheme = w;
	t.scrollBarTheme = w;
	t.tooltipTheme = w;
	t.menuTheme = w;
	t.menuItemTheme = w;

	t.sliderTheme.itemColor = bg;
	t.sliderTheme.innerColor = colorPlus(bg, nvgRGB(0x50, 0x50, 0x50));
	t.sliderTheme.innerSelectedColor = colorPlus(bg, nvgRGB(0x60, 0x60, 0x60));

	t.textFieldTheme = t.sliderTheme;
	t.textFieldTheme.textColor = colorMinus(bg, nvgRGB(0x20, 0x20, 0x20));
	t.textFieldTheme.textSelectedColor = t.textFieldTheme.textColor;

	t.scrollBarTheme.itemColor = colorPlus(bg, nvgRGB(0x50, 0x50, 0x50));
	t.scrollBarTheme.innerColor = bg;

	t.menuTheme.innerColor = colorMinus(bg, nvgRGB(0x10, 0x10, 0x10));
	t.menuTheme.textColor = colorMinus(fg, nvgRGB(0x50, 0x50, 0x50));
	t.menuTheme.textSelectedColor = t.menuTheme.textColor;

	bndSetTheme(t);
}

static int windowX = 0;
static int windowY = 0;
static int windowWidth = 0;
static int windowHeight = 0;

void windowSetFullScreen(bool fullScreen) {
	if (windowGetFullScreen()) {
		glfwSetWindowMonitor(gWindow, NULL, windowX, windowY, windowWidth, windowHeight, GLFW_DONT_CARE);
	}
	else {
		glfwGetWindowPos(gWindow, &windowX, &windowY);
		glfwGetWindowSize(gWindow, &windowWidth, &windowHeight);
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(gWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
}

bool windowGetFullScreen() {
	GLFWmonitor *monitor = glfwGetWindowMonitor(gWindow);
	return monitor != NULL;
}


////////////////////
// resources
////////////////////

Font::Font(const std::string &filename) {
	handle = nvgCreateFont(gVg, filename.c_str(), filename.c_str());
	if (handle >= 0) {
		info("Loaded font %s", filename.c_str());
	}
	else {
		warn("Failed to load font %s", filename.c_str());
	}
}

Font::~Font() {
	// There is no NanoVG deleteFont() function yet, so do nothing
}

std::shared_ptr<Font> Font::load(const std::string &filename) {
	static std::map<std::string, std::weak_ptr<Font>> cache;
	auto sp = cache[filename].lock();
	if (!sp)
		cache[filename] = sp = std::make_shared<Font>(filename);
	return sp;
}

////////////////////
// Image
////////////////////

Image::Image(const std::string &filename) {
	handle = nvgCreateImage(gVg, filename.c_str(), NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY);
	if (handle > 0) {
		info("Loaded image %s", filename.c_str());
	}
	else {
		warn("Failed to load image %s", filename.c_str());
	}
}

Image::~Image() {
	// TODO What if handle is invalid?
	nvgDeleteImage(gVg, handle);
}

std::shared_ptr<Image> Image::load(const std::string &filename) {
	static std::map<std::string, std::weak_ptr<Image>> cache;
	auto sp = cache[filename].lock();
	if (!sp)
		cache[filename] = sp = std::make_shared<Image>(filename);
	return sp;
}

////////////////////
// SVG
////////////////////

SVG::SVG(const std::string &filename) {
	printf("VCV SVG %s\n",filename.c_str());
	handle = nsvgParseFromFile(filename.c_str(), "px", SVG_DPI_VCV);
	if (handle) {
		info("Loaded SVG %s", filename.c_str());
	}
	else {
		warn("Failed to load SVG %s", filename.c_str());
	}
}

SVG::~SVG() {
	nsvgDelete(handle);
}

std::shared_ptr<SVG> SVG::load(const std::string &filename) {
	static std::map<std::string, std::weak_ptr<SVG>> cache;
	auto sp = cache[filename].lock();
	if (!sp)
		cache[filename] = sp = std::make_shared<SVG>(filename);
	return sp;
}


} // namespace rack
