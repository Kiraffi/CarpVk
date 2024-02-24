#pragma once

#ifndef ARRAYSIZES
#define ARRAYSIZES(theArray) (theArray ? (sizeof(theArray) / sizeof(theArray[0])) : 0)
#endif

#if _MSC_VER
#define DEBUG_BREAK_MACRO() __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK_MACRO() raise(SIGTRAP)
#endif

#define ASSERT_STRING(STUFF, STUFFSTRING) \
do { if (STUFF) {} else printf("Assertion: %s\n", STUFFSTRING); } while (0)

#define ASSERT(STUFF) ASSERT_STRING(STUFF, #STUFF)


using u32 = uint32_t;
