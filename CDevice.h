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

    Device(std::string name) : name(name) {}
    virtual ~Device() = default; // Allow dynamic casting w. destructor
};

#ifdef __linux__
struct LinuxDevice : Device {
    std::string eventHandle;
};
#endif

#ifdef TARGET_OS_MAC
struct MacDevice : Device {
    const IOHIDDeviceRef deviceRef;
    MacDevice(std::string name, const IOHIDDeviceRef deviceRef) : Device(name), deviceRef(deviceRef) {}
};
#endif

struct DeviceInfo {
    int64_t timestamp;
};

struct TabletDeviceInfo : DeviceInfo{
    bool isEngaged{};
    bool hasPressure{};
    int pressure{};
    int height{};
    int x{};
    int y{};
};