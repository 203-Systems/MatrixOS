#pragma once

// SYSTEM_APPLICATION
#include "applications/Shell/Shell.h"
#include "applications/Performance/Performance.h"
#include "applications/Note/Note.h"
#include "applications/REDACTED/REDACTED.h"
#include "applications/CustomKeymap/CustomKeymap.h"

// USER APPLICATION
#include "applications/Lighting/Lighting.h"
// #include "applications/HIDtest/HIDtest.h"
#include "applications/Example/Example.h"

// BOOT ANIMATION
#include "applications/Matrix/MatrixBoot/MatrixBoot.h"

// DEVICE APPLICATION
#include "applications/Matrix/FactoryMenu/FactoryMenu.h"

#define OS_SHELL APPID("203 Electronics", "Shell")

#define DEFAULT_BOOTANIMATION APPID("203 Electronics", "Matrix Boot")
