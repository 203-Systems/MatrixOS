#pragma once

// SYSTEM_APPLICATION
#include "applications/Shell/Shell.h"
#include "applications/Performance/Performance.h"
#include "applications/Note/Note.h"
#include "applications/REDACTED/REDACTED.h"
// #include "applications/CustomKeymap/CustomKeymap.h"

// USER APPLICATION
#include "applications/Lighting/Lighting.h"
#include "applications/Dice/Dice.h"

// BOOT ANIMATION
#include "applications/Matrix/MatrixBoot/MatrixBoot.h"

// DEVICE APPLICATION
#include "applications/Matrix/FactoryMenu/FactoryMenu.h"
#include "applications/Matrix/ForceCalibration/ForceCalibration.h"
#include "applications/Matrix/ForceGridVisualizer/ForceGridVisualizer.h"

#define OS_SHELL APPID("203 Electronics", "Shell")

#define DEFAULT_BOOTANIMATION APPID("203 Electronics", "Matrix Boot")
