#pragma once

// SYSTEM_APPLICATION
#include "applications/Shell/Shell.h"
#include "applications/Performance/Performance.h"
#include "applications/Note/Note.h"
#include "applications/REDACTED/REDACTED.h"
#include "applications/Companion/Companion.h"
#include "applications/CustomControlMap/CustomControlMap.h"

// USER APPLICATION
#include "applications/Lighting/Lighting.h"
#include "applications/Dice/Dice.h"
// #include "applications/Example/Example.h"
// #include "applications/Gamepad/Gamepad.h"
#include "applications/Reversi/Reversi.h"
#include "applications/PolyPlayground/PolyPlayground.h"
#include "applications/Strum/Strum.h"   

// BOOT ANIMATION
#include "applications/Mystrix/MystrixBoot/MystrixBoot.h"

// DEVICE APPLICATION
#include "applications/Mystrix/FactoryMenu/FactoryMenu.h"
#include "applications/Mystrix/ForceCalibration/ForceCalibration.h"

#define OS_SHELL APPID("203 Systems", "Shell")
#define DEFAULT_BOOTANIMATION APPID("203 Systems", "Mystrix Boot")
