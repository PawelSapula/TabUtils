#include <fcntl.h>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
// #include <ftxui/dom/elements.hpp>
#include <cstring>
#include <iostream>
#include <mutex>
#include <ostream>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <mach/mach.h>
#include <ftxui/screen/screen.hpp>

#include "CManager.h"
#ifdef __linux__
#include <linux/input.h>
#endif
#ifdef TARGET_OS_MAC
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#endif
#include <sys/poll.h>
#include "type_utils.h"
#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp"

using namespace ftxui;

#define _TUDEBUG 1
#if _TUDEBUG == 1
std::string sTUDebug{};
#define DEBUG_SHOW(str)  sTUDebug = str;
#else
#define DEBUG_SHOW()
#endif

/**
class FDDevice_IO {
    int m_Handle = -1;

private:
    void manageDeviceHandle(Device &device) {
        if (m_Handle > 0) {
            close(m_Handle);
        }
        std::string fd = "/dev/input/" + device.event;
        m_Handle = open(fd.c_str(), O_RDONLY);
    }

public:
    void init(Device &device) { manageDeviceHandle(device); }
    int getHandle() const { return m_Handle; }
};

#ifdef __linux__
void tabletEventConverter(input_event &ev, TabletDevice &tablet) {
    tablet.timestamp = ((int64_t) ev.time.tv_sec * 1000) + (ev.time.tv_usec / 1000); // ms conversion
    if (ev.type == EV_ABS) {
        if (ev.code == REL_X) { tablet.x = ev.value; }
        if (ev.code == REL_Y) { tablet.y = ev.value; }
        if (ev.code == ABS_PRESSURE) { tablet.pressure = ev.value; }
        if (ev.code == ABS_DISTANCE) { tablet.height = ev.value; }
    }
    if (ev.type == EV_KEY) {
        if (ev.code == BTN_TOOL_PEN) { tablet.isEngaged = ev.value; }
        if (ev.code == BTN_TOUCH) { tablet.hasPressure = ev.value; }
    }
}
#endif
#ifdef TARGET_OS_MAC


void callback(
    void * _Nullable context,
    IOReturn result,
    void * _Nullable sender,
    IOHIDReportType type,
    uint32_t reportID,
    uint8_t *report,
    CFIndex reportLength) {
    std::cout << static_cast<int *>(sender) << " " << reportLength << "\n";
    for (CFIndex i = 0; i < reportLength; i++) {
        std::cout << static_cast<int>(report[i]) << " ";
    }
    std::cout << "\n";
}

void valCallback(
    void * _Nullable context,
    IOReturn result,
    void * _Nullable sender,
    IOHIDValueRef value) {
    std::cout << value << std::endl;
}

*/
/**
std::vector<Device> getDeviceSpecifications() {
    IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone);
    if (!hidManager) {
        std::cerr << "Failed to create a HID Manager!" << std::endl;
    }

    IOHIDManagerSetDeviceMatching(hidManager, nullptr);
    //if (IOReturn result = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone); result != kIOReturnSuccess) {
    //  std::cerr << "Failed to open HID Manager! [" << mach_error_string(result) << "]" << std::endl;
    // CFRelease(hidManager);
    //}


    CFSetRef device_set = IOHIDManagerCopyDevices(hidManager);
    if (!device_set) {
        std::cerr << "Failed to access the devices" << std::endl;
    }


    std::vector<const void *> values(CFSetGetCount(device_set));
    CFSetGetValues(device_set, values.data());
    std::vector<Device> devices;
    CFRelease(device_set); // Unsure, for safety.

    /**
     *
     * System service accessing, these are high bandwith devices, not completely sure but kernel access?
     *
      CFMutableDictionaryRef  classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
      //CFMutableDictionaryRef matching = IOServiceMatching(kIOHIDSerialNumberKey);

      if (classesToMatch == NULL) {
        std::cerr << "IOService Return a NULL dictionary" << std::endl;
        exit(-1);
      }

      // Modems are more technical devices if you can say so, that support modem control lines (DTR, RTS, CTS, DCD)
      CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
        CFSTR(kIOSerialBSDModemType));

      io_iterator_t serialPortIterator;
      IOReturn res = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);
      if (res != KERN_SUCCESS) {
        std::cout << "IOServiceGetMatchingServices returned: " << res << std::endl;
      }

      io_object_t modemService;

      while ((modemService = IOIteratorNext(serialPortIterator))) {
        CFTypeRef path = IORegistryEntryCreateCFProperty(modemService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
        if (path) {
          char buf[256];
          if (CFStringGetCString(static_cast<CFStringRef>(path), buf, sizeof(buf), kCFStringEncodingUTF8)) {
            std::cout << buf << std::endl;
            CFRelease(path);
          }
        }
      }
      IOObjectRelease(modemService);
      IOObjectRelease(serialPortIterator);
    */


    /**
    IOHIDDeviceRef wacomDevice{};
    for (const void *value: values) {
        IOHIDDeviceRef device = static_cast<IOHIDDeviceRef>(const_cast<void *>(value));

        CFTypeRef productRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
        CFTypeRef manufacturerRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey));
        CFTypeRef maxReportSizeRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));
        //CFTypeRef usagePageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
        //CFTypeRef usageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));
        //CFTypeRef locationRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDLocationIDKey));

        if (!productRef && !manufacturerRef && !maxReportSizeRef) {
            std::cerr << "Properties not retrieved!" << std::endl;
            continue;
        }

        std::optional<std::string> product = convertTypeRef(productRef);
        std::optional<std::string> manufacturer = convertTypeRef(manufacturerRef);
        std::optional<std::string> maxReportSize = convertTypeRef(maxReportSizeRef);
        if (!product || !manufacturer || !maxReportSize) {
            std::cerr << "Failed to obtain device properties!" << std::endl;
        }

        std::string name = *product + ", " + *manufacturer + " (" + *maxReportSize + "-byte)";
        devices.push_back(Device{name, " "});

        if (name.contains("Wacom") && name.contains("10")) {
            // For testing purposes.
            wacomDevice = device;
            std::cout << name << "\n";
        }
    }

    if (!wacomDevice) {
        exit(-1);
    }
    */

    /** TODO: For now only System settings -> Privacy -> Input privileges work. Test around so that the user dont have to do it, this method may be necessary.
    IOHIDAccessType access = IOHIDCheckAccess(kIOHIDRequestTypeListenEvent);
    if (access != kIOHIDAccessTypeGranted) {
      std::cerr << "Access permissions not granted for the manager! Attempting request." << std::endl;
      IOReturn result = IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
      if (result != kIOReturnSuccess) {
        std::cerr << "Failed to obtain permissions. exiting.." << std::endl;
        exit(-1);
      }
      else {
        std::cerr << "Obtained permissions, continuing." << std::endl;
      }
    }
    */


        /*
    uint8_t report[512]{};
    CFIndex report_length = long(report);
    unsigned char report_id = 0;

    IOReturn res_open = IOHIDDeviceOpen(wacomDevice, kIOHIDOptionsTypeNone);
    if (res_open != kIOReturnSuccess) {
        std::cout << "Failed to open device! [" << mach_error_string(res_open) << std::endl;
        exit(-1);
    }
    */
    /**

    CFArrayRef elementArray = IOHIDDeviceCopyMatchingElements(device, nullptr, kIOHIDOptionsTypeNone);
    for (CFIndex i = 0; i < CFArrayGetCount(elementArray); i++ ) {
        void* rawElement = const_cast<void*>(CFArrayGetValueAtIndex(elementArray, i));
        IOHIDElementRef element = static_cast<IOHIDElementRef>(rawElement);
        std::cout << name << " ReportID " << IOHIDElementGetReportID(element) << std::endl;
        std::cout << name << " Report Size " << IOHIDElementGetReportSize(element) << std::endl;
        std::cout << name << " Re[prt count " << IOHIDElementGetReportCount(element) << std::endl;
        std::cout << name << " Usage page " << IOHIDElementGetUsagePage(element) << std::endl;
        std::cout << name << " Get usage " << IOHIDElementGetUsage(element) << std::endl;
    }

    //CFShow(elementArray);
    CFRelease(elementArray);
    */

    // OpenTabletDriver documentation to enable "recieving" mode or smth./
    /**
    uint8_t feature[] = {0x01, 0x02};

    IOReturn res = IOHIDDeviceSetReport(device, kIOHIDReportTypeFeature, 0x01, feature, sizeof(feature));
    if (res != kIOReturnSuccess) {
      std::cerr << "Sending feature report failed! [" << mach_error_string(res) << "]" << std::endl;
      std::cerr << std::hex << static_cast<uint32_t>(res) << std::dec << "\n";
    }
    */

        /*
    IOHIDDeviceRegisterInputReportCallback(wacomDevice, report, report_length, callback, nullptr);
    // Own context nullptr
    //IOHIDDeviceRegisterInputValueCallback(device,  valCallback, nullptr); // Own context nullptr
    IOHIDDeviceScheduleWithRunLoop(wacomDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // Append to current thread run loop
    CFRunLoopRun(); // Start loop


    CFRelease(hidManager);
    return devices;
}
#endif

#ifdef __linux__
std::vector<Devices> getDeviceSpecifications() {
    std::ifstream ifs("/proc/bus/input/devices");
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string device_list(buffer.str());


    int cursor = -1;

    std::vector<Device> devices;
    while (device_list.find('N: Name=\"', cursor + 1) != std::string::npos) // UNSAFE CODE LOOK BELOW HOW IT SHOULD BE
    {
        cursor = device_list.find('N: Name=\"', cursor + 1);
        cursor++; // Not include beginning quotation forward

        int end_quotation = device_list.find('"', cursor);
        std::string name = device_list.substr(cursor, end_quotation - cursor);

        cursor = device_list.find("H: Handlers=", end_quotation);
        cursor = device_list.find('=', cursor);
        cursor++;

        int handlers = device_list.find('\n', cursor);
        std::string eventHandles = device_list.substr(cursor, handlers - cursor);
        cursor = handlers + 1;

        std::stringstream ss(eventHandles);
        std::string handleName;
        while (ss >> handleName) {
            if (handleName.find("event") != std::string::npos) {
                devices.push_back(Device(name, handleName));
            }
        }
    }
    return devices;
}
#endif
*/
int main(int argc, char *argv[]) {

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("Help");
            return 0;
        }
    }

    DeviceManager::getDeviceList();
    //std::vector obtainedDevices(getDeviceSpecifications());
    //devices.push_back(Device{"Device", " ", true});
    //devices.reserve(devices.size() + obtainedDevices.size());
    //devices.insert(devices.end(), obtainedDevices.begin(), obtainedDevices.end());

    // Conversion to a string list
    std::vector<std::string> deviceNames;
    for (std::unique_ptr<Device>& device : DeviceManager::m_Devices) {
        deviceNames.push_back(device->name);
    }


    TabletDeviceInfo tabletDevice{};
    int iDevice = 0;
    int64_t polling_rate{};

    DropdownOption option;
    option.radiobox.entries = &deviceNames;
    option.radiobox.selected = &iDevice;

    //option.radiobox.on_change = [iDevice, &db] {
    //    Device& dev = *DeviceManager::m_Devices.at(iDevice);
    //    std::thread handleThread = DeviceManager::createHandleThread(dev);
    //    handleThread.detach();
    //    db = dev.name;
    //};

    auto deviceList = Dropdown(option);
    auto component = Renderer(deviceList, [&] {
        auto element = flexbox({
                           //text("Frame:" + std::to_string(frame)),
                           vbox({
                               text("TabUtils"),
                               text("Polling rate: " + std::to_string(tabletDevice.timestamp - polling_rate) + "ms"),
                               separator(),
                               text("Pen status: " + std::string(tabletDevice.isEngaged ? "Engaged" : "Disengaged")),
                               tabletDevice.isEngaged
                                   ? text("Height: " + std::to_string(tabletDevice.height))
                                   : text("No pen nearby"),
                               tabletDevice.hasPressure
                                   ? text("Pressure Strength: " + std::to_string(tabletDevice.pressure))
                                   : text("No pressure"),
                               tabletDevice.hasPressure ? gauge(tabletDevice.pressure / 2047.f) : emptyElement(),
                               // TODO: Delete magic number
                               separator(),
                               text("Abs. X: " + std::to_string(tabletDevice.x)),
                               text("Abs. Y: " + std::to_string(tabletDevice.y)),
                           }) | border,

                            text("Device Buffer: " + DeviceManager::m_Buffer) | border,
#if _TUDEBUG == 1
                            text("Debug: " + sTUDebug),
#endif

                           emptyElement() | flex_grow | borderEmpty,

                           vbox({
                               deviceList->Render(),
                           }),

                       }) | border;
        polling_rate = tabletDevice.timestamp;
        return element;
    });


    std::thread deviceListenerThread( [&]{
        int prevDevice = iDevice;
        std::thread currentHandleThread;

        while (true) {

            if (iDevice != prevDevice) {
                if (currentHandleThread.joinable() ) {
                    DeviceManager::cleanupHandleThread();
                    currentHandleThread.join();
                }

                Device& dev = *DeviceManager::m_Devices.at(iDevice);
                //DEBUG_SHOW(dev.name + " " + std::to_string(iDevice) + " " + std::to_string(prevDevice));
                currentHandleThread = DeviceManager::createHandleThread(dev);

                prevDevice = iDevice;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    });
    std::atomic<bool> running = true; // Atomic's are thread safe and race free.
/**
    std::thread inputThread([&] {
        while (iDevice == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Initial load
        FDDevice_IO fd_io;
        //input_event ev;
        int prevDevice = iDevice;

        pollfd pfd;
        fd_io.init(devices.at(iDevice));
        pfd.fd = fd_io.getHandle();
        pfd.events = POLLIN; // What event to look after

        while (running) {
            if (iDevice != prevDevice) {
                if (Device device = devices.at(iDevice); !device.placeholder) {
                    fd_io.init(devices.at(iDevice));
                    pfd.fd = fd_io.getHandle();
                }

                prevDevice = iDevice;
            }

            int result = poll(&pfd, 1, 100);

            if (result > 0 && (pfd.revents & POLLIN)) {
                //Returned events bitwise AND info in

                //read(pfd.fd, &ev, sizeof(ev));

                //tabletEventConverter(ev, tabletDevice);
            }
        }
    });
*/

    auto screen = ScreenInteractive::TerminalOutput();

    Loop loop(&screen, component);

    while (!loop.HasQuitted()) {
        screen.RequestAnimationFrame();
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    running = false;
    //inputThread.join();

    return 0;
}
