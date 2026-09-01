//
// Created by Pawel Sapula on 01/09/2026.
//

#pragma once
#include <string>

#ifdef TARGET_OS_MAC
#include <IOKit/hid/IOHIDManager.h>
#endif


struct Device {
    std::string name;
};

#ifdef __linux__
struct DeviceLinux : Device {
    std::string eventFD;
};
#endif

#ifdef TARGET_OS_MAC
struct MacDevice : Device {
    IOHIDDeviceRef deviceRef;
};
#endif

struct DeviceInfo {
    int64_t timestamp;
};

struct TabletDeviceInfo {
    bool isEngaged{};
    bool hasPressure{};
    int pressure{};
    int height{};
    int x{};
    int y{};
};