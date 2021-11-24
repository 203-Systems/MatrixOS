#pragma once

#define LOG_LEVEL_NONE      0
#define LOG_LEVEL_ERROR     1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_INFO      3
#define LOG_LEVEL_DEBUG     4
#define LOG_LEVEL_VERBOSE   5

enum ELogLevel : uint8_t 
{
    NONE =      LOG_LEVEL_NONE,
    ERROR =     LOG_LEVEL_ERROR,
    WARNING =   LOG_LEVEL_WARNING,
    INFO =      LOG_LEVEL_INFO,
    DEBUG =     LOG_LEVEL_DEBUG,
    VERBOSE =   LOG_LEVEL_VERBOSE
};