#include "util/common.hpp"
#include "asset.hpp"
#include <stdarg.h>


namespace rack {


static FILE *logFile = stderr;
static auto startTime = std::chrono::high_resolution_clock::now();


void loggerInit() {
#ifdef ARCH_WIN
	std::string logFilename = assetHidden("log.txt");
	logFile = fopen(logFilename.c_str(), "w");
#endif
}

void loggerDestroy() {
#ifdef ARCH_WIN
	fclose(logFile);
#endif
}

static void loggerLogVa(const char *type, const char *file, int line, const char *format, va_list args) {
	auto nowTime = std::chrono::high_resolution_clock::now();
	int duration = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime).count();
	fprintf(logFile, "[%.03f %s] ", duration / 1000.0, type);
	vfprintf(logFile, format, args);
	fprintf(logFile, "\n");
	fflush(logFile);
}

void loggerLog(const char *type, const char *file, int line, const char *format, ...) {
	va_list args;
	va_start(args, format);
	loggerLogVa(type, file, line, format, args);
	va_end(args);
}

/** Deprecated. Included for ABI compatibility */

#undef debug
#undef info
#undef warn
#undef fatal

void debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	loggerLogVa("debug", "", 0, format, args);
	va_end(args);
}

void info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	loggerLogVa("info", "", 0, format, args);
	va_end(args);
}

void warn(const char *format, ...) {
	va_list args;
	va_start(args, format);
	loggerLogVa("warn", "", 0, format, args);
	va_end(args);
}

void fatal(const char *format, ...) {
	va_list args;
	va_start(args, format);
	loggerLogVa("fatal", "", 0, format, args);
	va_end(args);
}


} // namespace rack
