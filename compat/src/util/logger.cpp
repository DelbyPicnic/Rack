#include "util/common.hpp"
#include "asset.hpp"
#include <stdarg.h>


namespace rack {

// static const char* const loggerText[] = {
// 	"debug",
// 	"info",
// 	"warn",
// 	"fatal"
// };

// static const int loggerColor[] = {
// 	35,
// 	34,
// 	33,
// 	31
// };

void loggerLog(LoggerLevel level, const char *file, int line, const char *format, ...) {
	// va_list args;
	// va_start(args, format);
	// loggerLogVa(level, file, line, format, args);
	// va_end(args);
}

/** Deprecated. Included for ABI compatibility */

#undef debug
#undef info
#undef warn
#undef fatal

void debug(const char *format, ...) {
	// va_list args;
	// va_start(args, format);
	// loggerLogVa(DEBUG_LEVEL, "", 0, format, args);
	// va_end(args);
}

void info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	// loggerLogVa(INFO_LEVEL, "", 0, format, args);
	vprintf(format, args);
	va_end(args);
}

void warn(const char *format, ...) {
	va_list args;
	va_start(args, format);
	// loggerLogVa(WARN_LEVEL, "", 0, format, args);
	vprintf(format, args);
	va_end(args);
}

void fatal(const char *format, ...) {
	// va_list args;
	// va_start(args, format);
	// loggerLogVa(FATAL_LEVEL, "", 0, format, args);
	// va_end(args);
}


} // namespace rack
