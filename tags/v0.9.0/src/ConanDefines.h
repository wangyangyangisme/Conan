/*!
    Conan - Connection Analyzer for Qt
    Copyright (C) 2008 - 2009 Elmar de Koning, edekoning@gmail.com

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


#ifndef _VERSION__05_01_09__11_22_08__H_
#define _VERSION__05_01_09__11_22_08__H_


#ifdef CONAN_EXPORTS
    #define CONAN_API __declspec(dllexport)
#else
    #define CONAN_API __declspec(dllimport)
#endif


//! String representation of the current version number.
#define CONAN_VERSION_STR "0.9.0"
//! The current version number: (major << 16) + (minor << 8) + patch.
#define CONAN_VERSION 0x000900
//! Used to check the current version against some other version: f.e. CONAN_VERSION >= CONAN_VERSION_CHECK(1, 0, 0)
#define CONAN_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))


//! The main namespace used for all code related to Conan
namespace conan {
}


#endif //_VERSION__05_01_09__11_22_08__H_