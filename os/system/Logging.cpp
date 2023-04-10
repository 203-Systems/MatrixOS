#include "MatrixOS.h"

#define DEFAULT_LOGGING_LEVEL LOG_VERBOSE  // Change this later

namespace MatrixOS::Logging
{

  void LogLevelSet(string tag, ELogLevel level) {}


  string logLevel[5] = {"E", "W", "I", "D", "V"};
  string logLevelColor[5] = {"31", "33", "32", "36", "37"};
  void Log(ELogLevel level, string tag, string format, va_list valst)  // DO NOT USE THIS DIRECTLY. STRING WILL NOT BE
                                                                       // REMOVED IF LOG LEVEL ISN'T SET FOR IT TO LOG
  {
#ifdef MATRIXOS_LOG_COLOR
      string msg = string() + "\033[0;" + logLevelColor[level - 1] + "m" + logLevel[level - 1] + " (" +
                   std::to_string(SYS::Millis()) + ") " + tag + ": " + format + "\033[0m\n";
#else
      string msg =
          string() + logLevel[level - 1] + " (" + std::to_string(SYS::Millis()) + ") " + tag + ": " + format + "\n";
#endif

#ifdef MATRIXOS_LOG_DEVICE
      Device::Log(msg, valst);
#endif

#ifdef MATRIXOS_LOG_USBCDC
      USB::CDC::VPrintf(msg, valst);
#endif
  }

  void LogError(string tag, string format, ...) {
#if MATRIXOS_LOG_LEVEL >= LOG_LEVEL_ERROR
    va_list valst;
    va_start(valst, format);
    Log(LOG_ERROR, tag, format, valst);
    va_end(valst);
#endif
  }

  void LogWarning(string tag, string format, ...) {
#if MATRIXOS_LOG_LEVEL >= LOG_LEVEL_WARNING
    va_list valst;
    va_start(valst, format);
    Log(LOG_WARNING, tag, format, valst);
    va_end(valst);
#endif
  }

  void LogInfo(string tag, string format, ...) {
#if MATRIXOS_LOG_LEVEL >= LOG_LEVEL_INFO
    va_list valst;
    va_start(valst, format);
    Log(LOG_INFO, tag, format, valst);
    va_end(valst);
#endif
  }

  void LogDebug(string tag, string format, ...) {
#if MATRIXOS_LOG_LEVEL >= LOG_LEVEL_DEBUG
    va_list valst;
    va_start(valst, format);
    Log(LOG_DEBUG, tag, format, valst);
    va_end(valst);
#endif
  }

  void LogVerbose(string tag, string format, ...) {
#if MATRIXOS_LOG_LEVEL >= LOG_LEVEL_VERBOSE
    va_list valst;
    va_start(valst, format);
    Log(LOG_VERBOSE, tag, format, valst);
    va_end(valst);
#endif
  }
}