#pragma once

#ifndef NDEBUG
#include <tracy/Tracy.hpp>
#define Profile() ZoneScoped
#else
#define Profile()
#endif