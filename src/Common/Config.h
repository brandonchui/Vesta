#pragma once

#include <stdint.h>
#include <stdlib.h>

//------------------------------------------------------------------------------------------------
// Platform
//------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
#define TARGET_WINDOWS
#elif defined(__APPLE__)
#define TARGET_MACOS
#else
#error "Unsupported platform"
#endif

//------------------------------------------------------------------------------------------------
// DLL IMPORT/EXPORT
//------------------------------------------------------------------------------------------------

// Ventus
#if defined(TARGET_WINDOWS)
#ifdef RHI_BUILD_DLL
#define VT_API __declspec(dllexport)
#else
#define VT_API __declspec(dllimport)
#endif
#elif defined(TARGET_MACOS) || defined(TARGET_LINUX)
#define VT_API __attribute__((visibility("default")))
#else
#define VT_API
#endif

//------------------------------------------------------------------------------------------------
// WINDOWS-SPECIFIC CONFIGURATION
//------------------------------------------------------------------------------------------------
#if defined(TARGET_WINDOWS)
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

//------------------------------------------------------------------------------------------------
// WINDOWS LIMITED SUPPORT
// support only d3d12 for now
//------------------------------------------------------------------------------------------------
#if defined(TARGET_WINDOWS)
#define D3D12
#endif

//------------------------------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------------------------------
#define ALIGNED_STRUCT(name, alignment)                                                            \
    typedef struct name name;                                                                      \
    __declspec(align(alignment)) struct name

//------------------------------------------------------------------------------------------------
// Options
//------------------------------------------------------------------------------------------------
#define ENABLE_DEBUG