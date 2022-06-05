//This file is for user modifiable variables
#pragma once

#include "framework/Framework.h"

#define USER_VAR_NAMESPACE "USER_VAR"
namespace MatrixOS::UserVar
{
    inline SavedVariable<EDirection> rotation(USER_VAR_NAMESPACE, "rotation", TOP);
    inline SavedVariable<uint8_t> brightness(USER_VAR_NAMESPACE, "brightness", 16);
    inline SavedVariable<Fract16> velocity_sensitive_threshold(USER_VAR_NAMESPACE, "velocity_sensitive_threshold", 0);
}
