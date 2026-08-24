#include <fcntl.h>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
// #include <ftxui/dom/elements.hpp>
#include <cstring>
#include <iostream>
#include <mutex>
#include <ostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <ftxui/screen/screen.hpp>
#include <linux/input.h>
#include <sys/poll.h>
#include <sys/syscall.h>

#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp"

using namespace ftxui;
typedef std::pair<std::string, std::string> ss_t;
using deviceInfo_t = std::vector<ss_t>;

std::string eventConverter(input_event *ev)
{
  if (ev->type == EV_ABS && ev->code == REL_X)
  {
    return "Absolute X: " + std::to_string(ev->value);
  }
  if (ev->type == EV_ABS && ev->code == ABS_Y)
  {
    return "Absolute Y: " + std::to_string(ev->value);
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOOL_PEN && ev->value == 1)
  {
    return "Pen engaged";
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOOL_PEN && ev->value == 0)
  {
    return "Pen disengaged";
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOUCH && ev->value == 1)
  {
    return "Pen touching";
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOUCH && ev->value == 0)
  {
    return "Pen not touching";
  }
  if (ev->type == ABS_DISTANCE || ev->code == ABS_DISTANCE) // 25
  {
    return "Hover: " + std::to_string(ev->value);
  }
  if (ev->type == ABS_PRESSURE || ev->code == ABS_PRESSURE) // 24
  {
    return "Touching strength: " + std::to_string(ev->value);
  }
  return "";
}

// Evdev checkout
void eventConverterA(input_event *ev, std::array<int, 6> &data)
{
  if (ev->type == EV_ABS && ev->code == REL_X)
  {
    data[4] = ev->value;
  }
  if (ev->type == EV_ABS && ev->code == ABS_Y)
  {
    data[5] = ev->value;
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOOL_PEN && ev->value == 1)
  {
    data[0] = 1;
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOOL_PEN && ev->value == 0)
  {
    data[0] = 0;
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOUCH && ev->value == 1)
  {
    data[1] = 1;
  }
  if (ev->type == EV_KEY && ev->code == BTN_TOUCH && ev->value == 0)
  {
    data[1] = 0;
  }
  if (ev->type == ABS_DISTANCE || ev->code == ABS_DISTANCE) // 25
  {
    data[2] = ev->value;
  }
  if (ev->type == ABS_PRESSURE || ev->code == ABS_PRESSURE) // 24
  {
    data[3] = ev->value;
  }
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "--help") == 0) {
      printf("Help");
      return 0;
    }
  }

  int fd = open("/proc/bus/input/devices", O_RDONLY);
  char buf[4096];
  std::string str;
  ssize_t bytesRead; // ssize_t can contain negatives, in case of errors
  while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
  str.append(buf, bytesRead);
  }

  close(fd);

  int cursor = -1;

  deviceInfo_t devices;
  devices.push_back(ss_t{"Devices", ""});

  while (str.find('N: Name=\"', cursor + 1) != std::string::npos) // UNSAFE CODE LOOK BELOW HOW IT SHOULD BE
  {
    cursor = str.find('N: Name=\"', cursor + 1);
    cursor++; // Not include beginning quotation forward

    int end_quotation = str.find('"', cursor);
    std::string name = str.substr(cursor, end_quotation-cursor);

    cursor = str.find("H: Handlers=", end_quotation);
    cursor = str.find('=', cursor);
    cursor++;

    int handlers = str.find('\n', cursor);
    std::string eventHandles = str.substr(cursor, handlers-cursor);
    cursor = handlers + 1;

    std::stringstream ss(eventHandles);
    std::string handleName;
    while (ss >> handleName)
    {
      if (handleName.find("event") != std::string::npos)
      {
        devices.push_back(ss_t{name, handleName});
      }
    }

  }

  //Allocate device names in a vector that FTXUI supports.
  std::vector<std::string> deviceNames;
  deviceNames.reserve(devices.size());
  for (ss_t device : devices)
  {
    deviceNames.push_back(device.first);
  }


std::atomic<bool> status = false;
std::atomic<bool> touching = false;
std::atomic<int> heightVal = 0;
std::atomic<int> touchStrengthVal = 0;
  std::atomic<int> x_pos = 0;
  std::atomic<int> y_pos = 0;

  int device_i = 0;
  int frame = 0;

  DropdownOption option;
  option.radiobox.entries = &deviceNames;
  option.radiobox.selected = &device_i;

  //TODO: IMPORTANT
  //TODO: Make this a function and execute it from the input thread, can cause disruptions in timing.

  std::atomic<int> fdEvent = -1;
  option.radiobox.on_change = [&]
  {
    close(fdEvent); //Important, or else it keeps making new ones. readlink /proc/<this pid>/fd/<fd>
    if (device_i == 0)
    {
      return;
    }

    std::string newFd = "/dev/input/" + devices.at(device_i).second;
    fdEvent = open(newFd.c_str(), O_RDONLY);
    if (fdEvent == -1)
    {
      exit(-1);
    }

  };


  auto deviceList = Dropdown(option);
  auto interactive = Container::Vertical({
    deviceList,
  });

  auto component = Renderer(deviceList, [&]
  {
    frame++;
    return flexbox({
      //text("Frame:" + std::to_string(frame)),
      vbox({
          text("TabUtils"),
      separator(),
      text("Pen status: " + std::string(status ? "Engaged" : "Disengaged")),
      status ? text("Height: " + std::to_string(heightVal)) : text("No pen nearby"),
      touching ? text("Touch Strength: " + std::to_string(touchStrengthVal)) : text("Not touching"),
      separator(),
      text("Abs. X: " + std::to_string(x_pos)),
      text("Abs. Y: " + std::to_string(y_pos)),
        //text(std::to_string(device_i) + " " + std::to_string(fdEvent) + " " +devices.at(device_i).second + std::to_string(getpid())), // For debugging
     }) | border,

      emptyElement() | flex_grow | borderEmpty,

      vbox({
    deviceList->Render(),
    }),

    }) | border;
  });

  std::atomic<bool> running = true; // Atomic's are thread safe and race free.

  std::thread inputThread([&]
 {
    while (fdEvent == -1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

   input_event ev;
   std::array<int, 6> data;

   pollfd pfd;
   pfd.events=POLLIN;

   while (running)
   {
     pfd.fd= fdEvent.load();
     int result = poll(&pfd, 1, 100);

     if (result > 0 && (pfd.revents & POLLIN)) {

     read(pfd.fd, &ev, sizeof(ev));

          eventConverterA(&ev, data);

           status = data[0];
           touching = data[1];
           heightVal = data[2];
           touchStrengthVal = data[3];
           x_pos = data[4];
           y_pos = data[5];

       /**std::string typeInfo;
       std::string codeInfo;

       switch (ev.type)
       {
         case EV_SYN:
         typeInfo = "Synchronous event";
         break;
       case EV_KEY:
         typeInfo = "Key device change event";
         break;
       default:
         typeInfo = std::to_string(ev.code);
         break;
       }

       switch (ev.code)
       {
       case ABS_DISTANCE:
         codeInfo = "Distance change";
         break;
       case REL_X:
         codeInfo = "X-Axis";
         break;
       case REL_Y:
         codeInfo = "Y-Axis";
         break;
       case 24:
         codeInfo = "Touching";
         break;
       default:
         codeInfo = std::to_string(ev.code);
         break;
       }

       if (ev.type != EV_SYN)
       {
         if (std::string formatted = eventConverter(&ev); !formatted.empty())
         {
           std::cout << formatted << std::endl;
           continue;
         }

         std::cout << (ev.time.tv_sec) << " Type: " << typeInfo << ", Code: " << codeInfo << ", (" << ev.code << ")" << ", Value: " << ev.value << std::endl;
       }
       **/

     }
   }
 });

  auto screen = ScreenInteractive::TerminalOutput();


  /**
  int counter = 0;
  auto comp = Renderer([&]
  {
    return text("coutner" + std::to_string(counter));
  });
*/

  Loop loop(&screen, component);

  while (!loop.HasQuitted())
  {
    //counter++;
    screen.RequestAnimationFrame();
    loop.RunOnce();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

 running = false;
  inputThread.join();

  close(fdEvent);

  return 0;
}
