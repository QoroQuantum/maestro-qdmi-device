/*------------------------------------------------------------------------------
Copyright 2025 Qoro Quantum Ltd.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
------------------------------------------------------------------------------*/

/**
 * @file Library.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The library class.
 *
 * Used to dynamically load a library on linux or windows.
 */

#pragma once

#ifndef _LIBRARY_H
#define _LIBRARY_H

#include <iostream>

#if defined(__linux__) || defined(__APPLE__)

#include <dlfcn.h>

#elif defined(_WIN32)

#include <windows.h>

#endif

namespace Utils {

class Library
{
public:
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
    Library(Library&&) = default;
    Library& operator=(Library&&) = default;

    Library() noexcept {}

    virtual ~Library()
    {
        if (handle)
#if defined(__linux__) || defined(__APPLE__)
            dlclose(handle);
#elif defined(_WIN32)
            FreeLibrary(handle);
#endif
    }

    virtual bool Init(const char* libName) noexcept
    {
#if defined(__linux__) || defined(__APPLE__)
        handle = dlopen(libName, RTLD_NOW);

        if (handle == nullptr) {
            const char* dlsym_error = dlerror();
            if (dlsym_error)
                std::cout << "Library: Unable to load library, error: " << dlsym_error << std::endl;

            return false;
        }
#elif defined(_WIN32)
        handle = LoadLibraryA(libName);
        if (handle == nullptr) {
            const DWORD error = GetLastError();
            std::cout << "Library: Unable to load library, error code: " << error << std::endl;
            return false;
        }
#endif

        return true;
    }

    void* GetFunction(const char* funcName) noexcept
    {
#if defined(__linux__) || defined(__APPLE__)
        return dlsym(handle, funcName);
#elif defined(_WIN32)
        return GetProcAddress(handle, funcName);
#endif
    }

    const void* GetHandle() const noexcept { return handle; }

private:
#if defined(__linux__) || defined(__APPLE__)
    void* handle = nullptr;
#elif defined(_WIN32)
    HINSTANCE handle = nullptr;
#endif
};

} // namespace Utils

#endif
