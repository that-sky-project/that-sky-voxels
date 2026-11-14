// Copyright (c) 2013-2016, Stoyan Nikolov
// All rights reserved.
// Voxels Library, please see LICENSE for licensing details.
#pragma once

#ifdef _WIN32

#ifdef VOXELS_DLL
#ifdef VOXELS_EXPORT
#define VOXELS_API __declspec(dllexport)
#else
#define VOXELS_API __declspec(dllimport)
#endif
#else
#define VOXELS_API
#endif

#ifndef VOXELS_CALL
#define VOXELS_CALL __stdcall
#endif

#else

#pragma error Voxels currently supports Windows only.

#endif