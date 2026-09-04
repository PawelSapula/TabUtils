//
// Created by Pawel Sapula on 03/09/2026.
//

#pragma once
#include "CDevice.h"
#include <vector>
#include <iostream>
#include <optional>
#include <mach/mach_error.h>

#ifdef TARGET_OS_MAC

#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>

#endif
#include "type_utils.h"
#include <thread>
#include <functional>

struct DeviceManager {
#ifdef TARGET_OS_MAC
   static IOHIDManagerRef m_HidManager; // TODO: Didnt fix to have same ref, can move to the mac methods and create sparately.
   static CFRunLoopRef m_RunLoop;
   ~DeviceManager() { CFRelease(m_HidManager); }
#endif
public:
   static std::vector<std::unique_ptr<Device>> m_Devices;
   static std::string m_Buffer;

   static void getDeviceList();
   static std::thread createHandleThread(Device& device);
   static void cleanupHandleThread();
};
