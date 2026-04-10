// This file is for user modifiable variables
#pragma once

#include "Framework/Framework.h"

#define USER_VAR_NAMESPACE "USER_VAR"
#define UserVar(name, type, default_value) inline CreateSavedVar(USER_VAR_NAMESPACE, name, type, default_value)

namespace MatrixOS::UserVar
{
// variable name, variable type, variable default
UserVar(deviceId, uint16_t, 0);

UserVar(brightness, uint8_t, 64);
UserVar(uiAnimation, bool, true);
UserVar(secretMenuEn, bool, false);

UserVar(fastScroll, bool, false);
} // namespace MatrixOS::UserVar
