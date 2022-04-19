#pragma once

#include "SystemApplications.h"

#ifdef DEVICE_APPLICATIONS
#include "DeviceApplications.h" // This file is inside device folder to load specific applications design for each device
#endif

#include "UserApplications.h"