//
// Created by Pawel Sapula on 30/08/2026.
//
#pragma once

#include <string>
#include <optional>

#ifdef TARGET_OS_MAC
#include <CoreFoundation/CoreFoundation.h>

std::optional<std::string> convertTypeRef(CFTypeRef& ref);
#endif