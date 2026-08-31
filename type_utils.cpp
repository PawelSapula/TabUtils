//
// Created by Pawel Sapula on 30/08/2026.
//

#include "type_utils.h"

std::optional<std::string> convertTypeRef(CFTypeRef& ref) {
    char buf[256]{};
    long long number{0xDEAD}; // TODO: Unsafe, fix later
    bool res = false;
    if (!ref) {
        return std::nullopt;
    }

    if (CFGetTypeID(ref) == CFStringGetTypeID()) {
        res = CFStringGetCString(static_cast<CFStringRef>(ref), buf, sizeof(buf), kCFStringEncodingUTF8);
    }
    if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
        res = CFNumberGetValue(static_cast<CFNumberRef>(ref), kCFNumberLongLongType, &number);
    }

    if (res) {
        if (number != 0xDEAD) {
            return std::to_string(number);
        }
        return std::string(buf);
    }
    return std::nullopt;
}
