#include "MatrixOS.h"
#include "printf.h"

#define DEFAULT_LOGGING_LEVEL VERBOSE //Change this later

namespace MatrixOS::Logging
{

    void LogLevelSet(string tag, ELogLevel level)
    {

    }

    ELogLevel GetLogLevel(string tag)
    {
        return DEFAULT_LOGGING_LEVEL;   
    }

    bool ShouldLog(string tag, ELogLevel target_level)
    {
        return (uint8_t)GetLogLevel(tag) >= (uint8_t)target_level;
    }

    string logLevelLut[5] = {"31mE", "33mW", "32mI", "36mD", "37mV"};
    void Log(ELogLevel level, string tag, string format, ...) //DO NOT USE THIS DIRECTLY. STRING WILL NOT BE REMOVED IF LOG LEVEL ISN'T SET FOR IT TO LOG
    {   
        if(ShouldLog(tag, level))
        {
            string msg = string() + "\033[0;" + logLevelLut[level - 1] + " (" + std::to_string(SYS::Millis()) + ") " +  tag + ": " + format + "\033[0m\n";
            va_list valst;
            #ifdef MATRIXOS_LOG_DEVICE
            Device::Log(msg, valst);
            #endif

            #ifdef MATRIXOS_LOG_USBCDC
            USB::CDC::Printf(msg, valst)
            #endif
        }

    }    


    void LogError (string tag, string format, ...)
    {
        #if MATRIXOS_LOG_LEVEL >= 1
        va_list valst;
        Log(ERROR, tag, format, valst);
        #endif
    }    

    void LogWarning (string tag, string format, ...)
    {
        #if MATRIXOS_LOG_LEVEL >= 2
        va_list valst;
        Log(WARNING, tag, format, valst);
        #endif
    }

    void LogInfo (string tag, string format, ...)
    {
        #if MATRIXOS_LOG_LEVEL >= 3
        va_list valst;
        Log(INFO, tag, format, valst);
        #endif
    }

    void LogDebug (string tag, string format, ...)
    {
        #if MATRIXOS_LOG_LEVEL >= 4
        va_list valst;
        Log(DEBUG, tag, format, valst);
        #endif
    }

    void LogVerbose (string tag, string format, ...)
    {
        #if MATRIXOS_LOG_LEVEL >= 5
        va_list valst;
        Log(VERBOSE, tag, format, valst);
        #endif
    }
}