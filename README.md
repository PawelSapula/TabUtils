# USBUtils
Tool for reading raw state of a USB Device.

# Summary 

Tool made with C++ utilising raw system libraries to obtain USB Device information (Often called HID - Human Interface Device).<br>
Originally ment to be a graphical-tablet only management app. <br><br>
Tested with Linux 7+ kernel and MacOS Tahoe 26.

> [!IMPORTANT]
> Utilization of POSIX and CoreFoundation proved challenging, for the project´s early stage, accessibility has been overlooked to speed up development. <br><br>
> **Running using MacOS:** System settings -> Privacy & Security -> Input Management -> Add terminal <br>
> **Running using Linux:** Superuser requirements to read file descriptors. Launch with `sudo ./USBUtils`

# Compilation & Running
- Clone repo
- Setup CMake directories: `cmake -S . -B <build folder dir>`
- Build with CMake: `cmake --build <build folder dir>`
- Access build directory and launch `./USBUtils`

# Gallery
<img width="595" height="239" alt="image" src="https://github.com/user-attachments/assets/cd2b45c5-da88-431a-823a-86a8b6178811" />
