#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <unordered_map>

namespace OffsetUpdater
{
    bool Initialize(bool keep_console_open = false);
}