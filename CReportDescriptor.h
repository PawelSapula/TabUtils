//
// Created by Pawel Sapula on 06/09/2026.
//

#pragma once
#include <map>

struct ReportDescriptor {
   int64_t timestamp;
   int location;
   std::map<int, int> bindings;
};

