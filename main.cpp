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
#include <linux/input.h>
#include <sys/poll.h>
#include <sys/syscall.h>
#include <libwacom-1.0/libwacom/libwacom.h>

#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp"

using namespace ftxui;

struct Device
{
  std::string name;
  std::string event;
  bool placeholder = false;
};

struct TabletDevice
{
  int64_t timestamp;
  bool isEngaged{};
  bool hasPressure{};
  int pressure{};
  int height{};
  int x{};
  int y{};
};

class FDDevice_IO
{
  int m_Handle = -1;
private:
  void manageDeviceHandle(Device& device)
  {
    if (m_Handle > 0)
    {
      close(m_Handle);
    }
      std::string fd = "/dev/input/" + device.event;
      m_Handle = open(fd.c_str(), O_RDONLY);
  }
public:
  void init(Device& device){manageDeviceHandle(device);}
  int getHandle() const { return m_Handle;}
};

void tabletEventConverter(input_event &ev, TabletDevice& tablet)
{
  tablet.timestamp = ((int64_t)ev.time.tv_sec*1000) + (ev.time.tv_usec/1000); // ms conversion
  if (ev.type == EV_ABS)
  {
    if (ev.code == REL_X) {tablet.x = ev.value;}
    if (ev.code == REL_Y) {tablet.y = ev.value;}
    if (ev.code == ABS_PRESSURE) {tablet.pressure = ev.value;}
    if (ev.code == ABS_DISTANCE) {tablet.height = ev.value; }
  }
  if (ev.type == EV_KEY)
  {
    if (ev.code == BTN_TOOL_PEN){tablet.isEngaged = ev.value;}
    if (ev.code == BTN_TOUCH){tablet.hasPressure = ev.value;}
  }
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "--help") == 0) {
      printf("Help");
      return 0;
    }
  }

  /**
  int fd = open("/proc/bus/input/devices", O_RDONLY);
  char buf[4096];
  std::string str;
  ssize_t bytesRead; // ssize_t can contain negatives, in case of errors
  while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
  str.append(buf, bytesRead);
  }
  close(fd) ;
  */

  std::ifstream ifs("/proc/bus/input/devices");
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  std::string device_list(buffer.str());


  int cursor = -1;

  std::vector<Device> devices;
  devices.push_back(Device{"Device", "", true});
  while (device_list.find('N: Name=\"', cursor + 1) != std::string::npos) // UNSAFE CODE LOOK BELOW HOW IT SHOULD BE
  {
    cursor = device_list.find('N: Name=\"', cursor + 1);
    cursor++; // Not include beginning quotation forward

    int end_quotation = device_list.find('"', cursor);
    std::string name = device_list.substr(cursor, end_quotation-cursor);

    cursor = device_list.find("H: Handlers=", end_quotation);
    cursor = device_list.find('=', cursor);
    cursor++;

    int handlers = device_list.find('\n', cursor);
    std::string eventHandles = device_list.substr(cursor, handlers-cursor);
    cursor = handlers + 1;

    std::stringstream ss(eventHandles);
    std::string handleName;
    while (ss >> handleName)
    {
      if (handleName.find("event") != std::string::npos)
      {
        devices.push_back(Device(name, handleName));
      }
    }

  }

  // Conversion to a string list
  std::vector<std::string> deviceNames;
  for (const Device& device : devices)
  {
    deviceNames.push_back(device.name);
  }



 TabletDevice tabletDevice;
  int iDevice = 0;
  int64_t polling_rate{};

  auto deviceList = Dropdown(&deviceNames, &iDevice);
  auto component = Renderer(deviceList, [&]
  {
    auto element = flexbox({
      //text("Frame:" + std::to_string(frame)),
      vbox({
          text("TabUtils"),
          text("Polling rate: " + std::to_string(tabletDevice.timestamp-polling_rate) + "ms"),
      separator(),
      text("Pen status: " + std::string(tabletDevice.isEngaged ? "Engaged" : "Disengaged")),
      tabletDevice.isEngaged ? text("Height: " + std::to_string(tabletDevice.height)) : text("No pen nearby"),
      tabletDevice.hasPressure ? text("Pressure Strength: " + std::to_string(tabletDevice.pressure)) : text("No pressure"),
        tabletDevice.hasPressure ? gauge(tabletDevice.pressure/2047.f) : emptyElement(), // TODO: Delete magic number
      separator(),
      text("Abs. X: " + std::to_string(tabletDevice.x)),
      text("Abs. Y: " + std::to_string(tabletDevice.y)),
     }) | border,

      emptyElement() | flex_grow | borderEmpty,

      vbox({
    deviceList->Render(),
    }),

    }) | border;
    polling_rate = tabletDevice.timestamp;
    return element;
  });

  std::atomic<bool> running = true; // Atomic's are thread safe and race free.

  std::thread inputThread([&]
 {
    while (iDevice == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Initial load
    FDDevice_IO fd_io;
    input_event ev;
    int prevDevice = iDevice;

   pollfd pfd;
    fd_io.init(devices.at(iDevice));
    pfd.fd = fd_io.getHandle();
   pfd.events=POLLIN; // What event to look after

   while (running)
   {
     if (iDevice != prevDevice)
     {
       if (Device device = devices.at(iDevice); !device.placeholder)
       {
         fd_io.init(devices.at(iDevice));
         pfd.fd = fd_io.getHandle();
       }

       prevDevice = iDevice;
     }

     int result = poll(&pfd, 1, 100);

     if (result > 0 && (pfd.revents & POLLIN)) { //Returned events bitwise AND info in

     read(pfd.fd, &ev, sizeof(ev));

       tabletEventConverter(ev, tabletDevice);

     }
   }
 });

  auto screen = ScreenInteractive::TerminalOutput();

  Loop loop(&screen, component);

  while (!loop.HasQuitted())
  {
    screen.RequestAnimationFrame();
    loop.RunOnce();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

 running = false;
  inputThread.join();

  return 0;
}
