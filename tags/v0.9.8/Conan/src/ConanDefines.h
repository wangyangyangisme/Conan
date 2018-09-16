/*!
    Conan - Connection Analyzer for Qt
    Copyright (C) 2008 - 2010 Elmar de Koning, edekoning@gmail.com

    This file is part of Conan.

    Conan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Conan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Conan.  If not, see <http://www.gnu.org/licenses/>.
*/

/*!
    \file
    \brief Contains Conan project related defines and version macros
*/


#ifndef _CONANDEFINES__05_01_09__11_22_08__H_
#define _CONANDEFINES__05_01_09__11_22_08__H_


#include <QtCore/qglobal.h>


#if QT_VERSION < QT_VERSION_CHECK (4, 6, 0) || QT_VERSION > QT_VERSION_CHECK (4, 7, 0)
#error "CONAN currently only supports Qt 4.6.0 - 4.7.0"
#endif


// Generic helper definitions for shared library support (see http://gcc.gnu.org/wiki/Visibility for details)
#if defined _WIN32 || defined __CYGWIN__
  #define CONAN_HELPER_DLL_IMPORT __declspec(dllimport)
  #define CONAN_HELPER_DLL_EXPORT __declspec(dllexport)
  #define CONAN_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define CONAN_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))
    #define CONAN_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))
    #define CONAN_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define CONAN_HELPER_DLL_IMPORT
    #define CONAN_HELPER_DLL_EXPORT
    #define CONAN_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define CONAN_API and CONAN_LOCAL.
// CONAN_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// CONAN_LOCAL is used for non-api symbols.

#ifdef CONAN_DLL // defined if Conan is compiled as a DLL
  #ifdef CONAN_DLL_EXPORTS // defined if we are building the Conan DLL (instead of using it)
    #define CONAN_API CONAN_HELPER_DLL_EXPORT
  #else
    #define CONAN_API CONAN_HELPER_DLL_IMPORT
  #endif // CONAN_DLL_EXPORTS
  #define CONAN_LOCAL CONAN_HELPER_DLL_LOCAL
#else // CONAN_DLL is not defined: this means Conan is a static lib.
  #define CONAN_API
  #define CONAN_LOCAL
#endif // CONAN_DLL


//! String representation of the current version number.
#define CONAN_VERSION_STR "0.9.8"
//! The current version number: (major << 16) + (minor << 8) + patch.
#define CONAN_VERSION 0x000908
//! Used to check the current version against some other version: f.e. CONAN_VERSION >= CONAN_VERSION_CHECK(1, 0, 0)
#define CONAN_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))


//! The main namespace used for all code related to Conan
namespace conan {
}


#endif //_CONANDEFINES__05_01_09__11_22_08__H_
