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

#define ASSERT(STUFF) ASSERT_STRING(STUFF, #STUFF)

#define MIN_VALUE(a, b) (a < b ? a : b)
#define MAX_VALUE(a, b) (a > b ? a : b)

#define VK_CHECK_CALL(call) do { \
    VkResult callResult = call; \
    if(callResult != VkResult::VK_SUCCESS) \
        printf("Result: %i\n", int(callResult)); \
    ASSERT(callResult == VkResult::VK_SUCCESS); \
    } while(0)

using u32 = uint32_t;
