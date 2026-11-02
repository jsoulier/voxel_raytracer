#pragma once

#if TRACY_ENABLE
#include <tracy/Tracy.hpp>

#define Profile() ZoneScoped
#define ProfileBlock(name) ZoneScopedN(name)
#define ProfileFrame() FrameMark
#else
#define Profile()
#define ProfileBlock(name)
#define ProfileFrame()
#endif
