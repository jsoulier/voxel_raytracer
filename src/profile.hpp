#pragma once

#ifndef NDEBUG
#include <tracy/Tracy.hpp>
#define Profile() ZoneScoped
#define ProfileBlock(name) ZoneScopedN(name)
#else
#define Profile()
#define ProfileBlock(name)
#endif