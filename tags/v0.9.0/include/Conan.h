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
    \brief Main include file for using Conan in other projects
*/


#include <qglobal.h>


#if QT_VERSION < QT_VERSION_CHECK (4, 4, 3)
#error "CONAN currently only supports Qt 4.4.3"
#endif


#include "../src/ConanWidget.h"


typedef conan::ConanWidget ConanWidget;


/*!
    \mainpage Conan - Connection Analyzer for Qt
    
    \image html Conan.jpg
    
    \section Conan
    Conan is a C++ library that provides run-time introspection of signal/slot
    connections. \n It provides a QWidget that can discover existing QObject
    hierarchies and display all signals, \n slots and active connections for
    any QOject. \n
    \n
    Contact - edekoning@gmail.com \n
    Website - http://sourceforge.net/projects/conanforqt \n
    License - GPL (source code); CCL (icons) \n
    \n
    \section Usage
    \code
    #include <Conan.h>                      // checks for proper qt version and
                                            // pulls ConanWidget into the
                                            // global namespace
    ConanWidget widget;
    widget.AddRootObject (myMainWindow);    // optional
    widget.AddRootObject (someOtherObject); // optional
    widget.show ();
    \endcode
*/
