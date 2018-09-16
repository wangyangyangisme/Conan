Conan 0.9.7 - Connection analyzer for Qt 4.6.0 - Qt 4.6.3
---------------------------------------------------------

    Conan is a C++ library that provides run-time introspection of object
    hierarchies, object inheritance, signal/slot connections, and signal
    emissions.

    Conan contains a single widget (conan::ConanWidget) that provides the
    following functionality:
    -object hierarchies can be added, discovered, browsed and exported to XML
    -signals/slots, including active connections, can be examined
    -duplicate connections can be found
    -signal emissions can be monitored and logged
    See /doc/html/index.html for detailed information.


    Contact - edekoning@gmail.com
    Website - http://sourceforge.net/projects/conanforqt
    License - GPL - source code -> see ./GPL.txt
              CCL - icons -> see ./icons/CCL.txt


    Conan was made using the following tools:
    -Silk icons made by Mark James available at FamFamFam
        http://www.famfamfam.com/lab/icons/silk/
    -Qt SDK Open Source Edition, available from Nokia
        http://qt.nokia.com/downloads/
    -Visual C++ 2008 Express Edition, available from Microsoft
        http://www.microsoft.com/express/vc/
    -Doxygen by Dimitri van Heesch, for source code documentation
        http://www.doxygen.org/
    -SourceForge, which hosts this project
        http://sourceforge.net/
        
    Feel free to contact me for suggestions, remarks, bugs, feature requests,
    or anything else Conan related.


Conan demo package
------------------

    The Conan demo package contains an application that demonstrates the use of
    Conan. It shows the conan::ConanWidget and a console window that captures
    the output of the signal loggers. The conan::ConanWidget contains itself as
    root object, so it can be used to examine to internals of Conan.

    For more information about the use of the conan::ConanWidget see the
    generated documentation /doc/index.html.

    The Conan Demo application has been tested on:
        -Windows XP SP3
        -Windows Vista SP2 32bit
        -Windows 7 32bit


Conan source package - installation
-----------------------------------

    Conan provides both a Visual Studio 2008 project file (/win32 folder) and a
    pro file (/conan.pro) for use with QT Creator and qmake. Whichever you
    choose, make sure the following environment changes have been made:
        QTDIR
            the root dir of where QT has been installed to
        QMAKESPEC
            should be set to the platform your compiling for, f.e.:
            win32-msvc2008. The qmakespec is used to include the proper
            platform specific private qt headers.

    The following paths are used to find all Qt headers used by Conan:
        $(QTDIR)/include
        $(QTDIR)/mkspecs/$(QMAKESPEC)
        $(QTDIR)/src

    All sources are located in /src folder, including generated moc, ui and qrc
    files. With Visual Studio these files can be regenerated using the build
    rules as specified in the QTRules.rules file, located in the /win32 folder.

    Note that Conan uses parts of the private non-documented Qt-API. This is
    needed to be able to retrieve all the connection information. As such there
    is no guarentee that Conan will continue to build after switching to a
    different (non-supported) Qt version. Use of the private Qt-API has been
    restricted to a single header: /src/ConanWidget_p.h

    In case you are not using the Open Source Edition of Qt, it could be that
    you do not have access to the complete Qt source code. Normally these
    sources are located in $(QTDIR)/src. The file /src/ConanWidget_p.h includes
    the following Qt source files:
        $(QTDIR)/src/corelib/kernel/qobject_p.h
        $(QTDIR)/src/corelib/kernel/qmetaobject_p.h
    If you are missing these files I suggest downloading the appropriate Qt
    Open Source Edition and using those sources.


Using Conan
-----------

    After building Conan (see installation), link your project against the
    generated import library Conan(d).lib and make sure your application can
    find the Conan(d).dll. Furthermore, the /include folder should be part of
    your include search path.

    Make sure QT_NO_DEBUG_OUTPUT is not defined as it will disable signal spy
    logging.

    To create and show the Conan widget in your application, all you need to do
    is add the following lines of code:

    #include <Conan.h>          // checks for proper qt version and pulls
                                // ConanWidget into the global namespace

    Q_INIT_RESOURCE (Conan);    // necessary on some platforms when Conan
                                // is build as a static library
    ConanWidget widget;
    widget.AddRootObject (myMainWindow);    // optional
    widget.AddRootObject (someOtherObject); // optional
    widget.show ();

    For more information about the use of the Conan widget see the generated
    code documentation /doc/html/index.html.

Copyright (C) 2008 - 2010 Elmar de Koning, All Rights Reserved
