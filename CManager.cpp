//
// Created by Pawel Sapula on 03/09/2026.
//

#include "CManager.h"

#ifdef __linux__
#define OSDEVICE LinuxDevice
static std::vector<LinuxDevice> getSystemDeviceList() {
    std::ifstream ifs("/proc/bus/input/devices");
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string device_list(buffer.str());


    int cursor = -1;

    std::vector<Device> devices{};
    while (device_list.find("N: Name=", cursor + 1) != std::string::npos) // UNSAFE CODE LOOK BELOW HOW IT SHOULD BE
    {
        cursor = device_list.find("N: Name=", cursor + 1);
        cursor = device_list.find('='), cursor;
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
                devices.push_back(LinuxDevice(name, handleName));
            }
        }
    }
    return devices;
}
static std::thread createSystemHandleThread(LinuxDevice& device) {
    std::string fd = "/dev/input/" + device.eventHandle;

    int handle = open(fd.c_str(), O_RDONLY);
    input_event ev;

    pollfd pfd;
    pfd.fd = handle;
    pfd.events = POLLIN; // What event to look after

    while (running) {

        int result = poll(&pfd, 1, 100);

        if (result > 0 && (pfd.revents & POLLIN)) {
            //Returned events bitwise AND info in

            input_event ev = read(pfd.fd, &ev, sizeof(ev));
            std::cout << ev << "/n";

        }
    }

}
#endif

#ifdef TARGET_OS_MAC
#define OSDEVICE MacDevice
static std::vector<MacDevice> getSystemDeviceList() {

DeviceManager::m_HidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone);
    if (!DeviceManager::m_HidManager) {
        std::cerr << "Failed to create a HID Manager!" << std::endl;
        return {};
    }
    IOHIDManagerSetDeviceMatching(DeviceManager::m_HidManager, nullptr);


    CFSetRef device_set = IOHIDManagerCopyDevices(DeviceManager::m_HidManager);
    if (!device_set) {
        std::cerr << "Failed to access the devices" << std::endl;
        return {};
    }


    std::vector<const void *> values(CFSetGetCount(device_set));
    CFSetGetValues(device_set, values.data());
    CFRelease(device_set); // Unsure, for safety.

    std::vector<MacDevice> devices;

    for (const void *value: values) {
        IOHIDDeviceRef device = static_cast<IOHIDDeviceRef>(const_cast<void *>(value));

        CFTypeRef productRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
        CFTypeRef manufacturerRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey));
        CFTypeRef maxReportSizeRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));
        //CFTypeRef usagePageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
        //CFTypeRef usageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));
        //CFTypeRef locationRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDLocationIDKey));

        if (!productRef && !manufacturerRef && !maxReportSizeRef) {
            //std::cerr << "Properties not retrieved!" << std::endl;
            continue;
        }

        std::optional<std::string> product = convertTypeRef(productRef);
        std::optional<std::string> manufacturer = convertTypeRef(manufacturerRef);
        std::optional<std::string> maxReportSize = convertTypeRef(maxReportSizeRef);
        if (!product || !manufacturer || !maxReportSize) {
            //std::cerr << "Failed to obtain device properties!" << std::endl;
            continue;
        }

        std::string name = *product + ", " + *manufacturer + " (" + *maxReportSize + "-byte)";

        MacDevice macDevice(name, device);
        devices.push_back(macDevice);

    }

    return devices;
}

static void createSystemHandleThread(Device &dev) {
    MacDevice& device = dynamic_cast<MacDevice&>(dev);

    uint8_t report[512]{};
    CFIndex report_length = long(report);

    IOReturn res_open = IOHIDDeviceOpen(device.deviceRef, kIOHIDOptionsTypeNone);
    if (res_open != kIOReturnSuccess) {
        std::cout << "Failed to open device! [" << mach_error_string(res_open) << std::endl;
        exit(-1);
    }

    IOHIDDeviceRegisterInputReportCallback(device.deviceRef, report, report_length,
        [](void *context, IOReturn result, void *sender, IOHIDReportType type, uint32_t reportID, uint8_t *report, CFIndex reportLength) {

            //std::cout << static_cast<int *>(sender) << " " << reportLength << "\n";
                std::string buf{};
            for (CFIndex i = 0; i < reportLength; i++) {
                buf.append(" " + std::to_string(static_cast<int>(report[i])));
            }
            DeviceManager::m_Buffer = buf;

        }
        , nullptr); // Own context nullptr

    IOHIDDeviceScheduleWithRunLoop(device.deviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // Append to current thread run loop
    DeviceManager::m_RunLoop = CFRunLoopGetCurrent(); // Save reference
    CFRunLoopRun(); // Start loop
    DeviceManager::m_RunLoop = nullptr; // Cleanup
}

static void cleanupSystemHandleThread() {
    if (DeviceManager::m_RunLoop != nullptr) {
        CFRunLoopStop(DeviceManager::m_RunLoop);
    }
}
#endif


#ifdef TARGET_OS_MAC
IOHIDManagerRef DeviceManager::m_HidManager;
CFRunLoopRef DeviceManager::m_RunLoop;
#endif

std::vector<std::unique_ptr<Device>> DeviceManager::m_Devices{};
std::string DeviceManager::m_Buffer{};

void DeviceManager::getDeviceList() {
        const auto sysDevices = getSystemDeviceList();

        for (const auto& device : sysDevices) {
            auto ptr = std::make_unique<OSDEVICE>(device);
            m_Devices.push_back(std::move(ptr));
        }
}

std::thread DeviceManager::createHandleThread(Device& device) {
    return std::thread(createSystemHandleThread, std::ref(device));
}

void DeviceManager::cleanupHandleThread() {
    return cleanupSystemHandleThread();
}
