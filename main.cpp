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
#include <mach/mach.h>
#endif
#include <sys/poll.h>
#include "type_utils.h"
#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp"

using namespace ftxui;

#define _TUDEBUG 1
#if _TUDEBUG == 1
std::string sDebug{};
#define DEBUG_SHOW(str)  sDebug = str;
#else
#define DEBUG_SHOW()
#endif

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

int main(int argc, char *argv[]) {

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("Help");
            return 0;
        }
    }

    DeviceManager::getDeviceList();

    // Conversion to a string list
    std::vector<std::string> deviceNames;
    for (std::unique_ptr<Device>& device : DeviceManager::m_Devices) {
        deviceNames.push_back(device->name);
    }


    TabletDeviceInfo tabletDevice{}; // Temporary holder
    int iDevice = 0;
    int64_t polling_rate{};

    auto deviceList = Dropdown(deviceNames, &iDevice);
    auto component = Renderer(deviceList, [&] {
        std::string buffer;
        {
            std::lock_guard lock(DeviceManager::m_Buffer_mutex);
            buffer = DeviceManager::m_Buffer;
        }
        auto element = flexbox({
                           vbox({
                               text("USBUtils"),
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

                            text("Device Buffer: " + buffer ) | border,
#if _TUDEBUG == 1
                            text("Debug: " + sDebug),
#endif

                           emptyElement() | flex_grow | borderEmpty,

                           vbox({
                               deviceList->Render(),
                           }),

                       }) | border;
        polling_rate = tabletDevice.timestamp;
        return element;
    });

    std::atomic<bool> running = true; // Atomic's are thread safe and race free.
    std::thread deviceListenerThread( [&]{
        int prevDevice = iDevice;
        std::thread currentHandleThread;

        while (running) {

            if (iDevice != prevDevice) {
                if (currentHandleThread.joinable() ) {
                    DeviceManager::cleanupHandleThread();
                    currentHandleThread.join();
                }

                Device& dev = *DeviceManager::m_Devices.at(iDevice);
                DEBUG_SHOW(dev.name + " " + std::to_string(iDevice) + " " + std::to_string(prevDevice));
                currentHandleThread = DeviceManager::createHandleThread(dev);

                prevDevice = iDevice;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    });

    auto screen = ScreenInteractive::TerminalOutput();

    Loop loop(&screen, component);

    while (!loop.HasQuitted()) {
        screen.RequestAnimationFrame();
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    running = false;
    deviceListenerThread.join();

    return 0;
}
