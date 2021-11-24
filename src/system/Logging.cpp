#include "MatrixOS.h"
#include "printf.h"

#define DEFAULT_LOGGING_LEVEL VERBOSE

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

    void Log(ELogLevel level, string tag, string format, ...) //DO NOT 
    {   
        if(ShouldLog(tag, level))
        {
            va_list valst;
            #ifdef MATRIXOS_LOG_DEVICE
            Device::Log(level, tag, format, valst);
            #endif

            #ifdef MATRIXOS_LOG_USBCDC
            USB::CDC::Printf(tag + " Info: " + format, valst)
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