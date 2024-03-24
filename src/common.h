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
do { if (STUFF) {} else { printf("Assertion: %s\n", STUFFSTRING); /*DEBUG_BREAK_MACRO();*/} }  while (0)

#define ASSERT_STRING_RETURN_IF_FALSE(STUFF, STUFFSTRING) \
do { if (STUFF) {} else { printf("Assertion: %s\n", STUFFSTRING); /*DEBUG_BREAK_MACRO();*/ return false; } }  while (0)

#define ASSERT(STUFF) ASSERT_STRING(STUFF, #STUFF)

#define ASSERT_RETURN_IF_FALSE(STUFF) ASSERT_STRING_RETURN_IF_FALSE(STUFF, #STUFF)

#define MIN_VALUE(a, b) (a < b ? a : b)
#define MAX_VALUE(a, b) (a > b ? a : b)

using u32 = uint32_t;
