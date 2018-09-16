Conan 0.9.0 - Connection analyzer for Qt 4.4.3
----------------------------------------------

    Conan is a C++ library that provides run-time introspection of signal/slot
    connections. It provides a QWidget that can discover existing QObject
    hierarchies and display all signals, slots and active connections for any
    QOject.

    Contact - edekoning@gmail.com
    Website - http://sourceforge.net/projects/conanforqt
    License - GPL - source code -> see ./GPL.txt
              CCL - icons -> see ./icons/CCL.txt


    Main features:
    -Add and view multiple object hierarchies or discover them automatically
    -Search object hierarchies by class name, object name or object address
    -Provides detailed information for all signals/slots of any object:
        -Access specifier (public, protected, private)
        -Method signature
        -Name of the class that declares the signal/slot
        -Lists all connections for each signal/slot:
            -Receiver method signature
            -Receiver class name and object name
            -Receiver object address
            -Connection type (auto, direct, queued, blocking queued)
    -Hide signals/slots without connections
    -Hide inherited signals/slots
    -Provides inheritance hierarchy for any object


    Conan was made using the following tools:
    -Silk icons made by Mark James available at FamFamFam
        http://www.famfamfam.com/lab/icons/silk/
    -Qt Open Source Edition, available from Trolltech
        http://trolltech.com/downloads/opensource/
    -Visual C++ 2008 Express Edition, available from Microsoft
        http://www.microsoft.com/express/vc/
    -Doxygen by Dimitri van Heesch, for source code documentation
        www.doxygen.org/
    -SourceForge, which hosts this project
        http://sourceforge.net/


Installation
------------

    Conan currently only provides Visual Studio 2008 project files. However,
    the following section should provide enough information on building Conan
    for your platform of choice. The next release will hopefully provide make
    files and or pro files (for use with qmake). 

    The /win32 folder contains the Conan.vcproj project file for use in Visual
    Studio 2008. Before opening this project make sure the following
    environment changes have been made:
        QTDIR => the root dir of where qt has been installed to
        QMAKESPEC => should be set to the platform your compiling for, f.e.:
        win32-msvc2008. The qmakespec is used to include the proper platform
        specific private qt headers.

    The following paths are used to find all Qt headers, libs and dlls:
        $(QTDIR)/include
        $(QTDIR)/include/qtgui
        $(QTDIR)/include/qtcore
        $(QTDIR)/mkspecs/$(QMAKESPEC)
        $(QTDIR)/lib
        $(QTDIR)/bin

    The project contains a debug and release build target that generate the
    Conan(d).dll (/bin folder) and import library Conan(d).lib (/lib folder).

    All sources are located in /src folder, including generated moc, ui and qrc
    files. These files can be regenerated using the build rules as specified in
    the QTRules.rules files, located in the /win32 folder.

    Note that Conan uses parts of the private non-documented Qt-API. This is
    needed to be able to retrieve all the connection information. As such there
    is no guarentee that Conan will continue to build after switching to a
    different (non-supported) Qt version. Use of the private Qt-API has been
    restricted to a single header: ConanWidget_p.h


Using Conan
-----------

    After building Conan (see installation), link your project against the
    generated import library Conan(d).lib and make sure your application can
    find the Conan(d).dll. Furthermore, the /include folder should be part of
    your include search path.

    To create and show the Conan widget in your application, all you need to do
    is add the following lines of code:

    #include <Conan.h>                      // checks for proper qt version and
                                            // pulls ConanWidget into the
                                            // global namespace
    ConanWidget widget;
    widget.AddRootObject (myMainWindow);    // optional
    widget.AddRootObject (someOtherObject); // optional
    widget.show ();

    For more information about the use of the Conan widget see the generated
    code documentation in /doc/html.


Copyright (C) 2008 - 2009 Elmar de Koning, All Rights Reserved