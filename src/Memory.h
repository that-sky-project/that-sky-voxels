// Copyright (c) 2013-2016, Stoyan Nikolov
// All rights reserved.
// Voxels Library, please see LICENSE for licensing details.
#pragma once

void* __cdecl operator new(std::size_t size);
void* __cdecl operator new(std::size_t count, const std::nothrow_t& tag) noexcept;
void* __cdecl operator new[](std::size_t count, const std::nothrow_t& tag) noexcept;
void* __cdecl operator new[](std::size_t size);
void __cdecl operator delete(void* ptr);
void __cdecl operator delete(void* ptr, const std::nothrow_t& tag) noexcept;
void __cdecl operator delete[](void* ptr);
void __cdecl operator delete[](void* ptr, const std::nothrow_t& tag) noexcept;
