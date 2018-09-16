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
    \brief Contains SignalSpy related declarations
*/


#ifndef _SIGNALSPY__31_03_09__01_53_37__H_
#define _SIGNALSPY__31_03_09__01_53_37__H_


#include "ConnectionModel.h"
#include <QtCore/QAbstractTableModel>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtGui/QTableView>
#include <vector>


namespace conan {

    class SignalLogger;

    /*!
        \brief The SignalSpy class enables introspection of signal emission.

        SignalSpy can connect to any signal of any object and output each emission using qDebug.
        Note that this class is an adaptation of the existing QSignalSpy class.
    */
    class SignalSpy : public QObject
    {
    public:
        SignalSpy (const QObject* inObject, const QString& inSignal, SignalLogger* inLogger);

        int qt_metacall (QMetaObject::Call inCall, int inMethodId, void **inArgs);

        const MethodData& SignalData () const;
        int EmitCount () const;

    private:
        void InitArgs (const QMetaMethod& inMember);
        void ProcessArgs (void **inArgs);

    private:
        SignalLogger* mLogger;      //!< Performs the actual logging of each signal emission
        QList <int> mArgTypes;      //!< The QMetaType types for the argument list of the signal
        MethodData mSignalData;     //!< Information about the signal being spied
        unsigned mEmitCount;        //!< The number of times the signal has been emitted
    };


    // --------------------------------------------------------------------------------------------


    /*!
        \brief Controls the logging of signal emissions.
        
        The SignalLogger uses the following log format:
        <timestamp> | <object> | <address> | <signature> | #<emit count> | <signal emission arguments>
        
        Each individual part of the log can be enabled or disabled. Furthermore, the width of each
        part can be fixed to a custom value or kept dynamic. Fixed fields are truncated when the
        text is too long or padded with spaces when the text is too short.
        
        All logging is performed using qDebug calls.
    */
    class SignalLogger
    {
    public:
        static const unsigned kDynamicWidth = 0;    //!< Indicates a dynamic field width

        SignalLogger ();

        void Log (SignalSpy* inSpy, const QString& inArgs) const;
        QString GetExampleLog () const;

    private:
        QString BuildLogMessage (const MethodData& inSignalData, int inEmitCount, const QString& inArgs) const;

    public:
        bool mShowTimestamp;        //!< Controls timestamp field
        bool mShowObject;           //!< Controls object field
        bool mShowAddress;          //!< Controls address field
        bool mShowSignature;        //!< Controls signal signature field
        bool mShowEmitCount;        //!< Controls signal emit count field
        bool mShowArguments;        //!< Controls arguments field

        unsigned mObjectWidth;      //!< object field width
        unsigned mSignatureWidth;   //!< signal signature field width
        unsigned mEmitCountWidth;   //!< signal emit count field width

        bool mPrettyFormatting;     //!< indicates if log message fields should be alligned between logs
        QChar mSeparator;           //!< the character used for seperating log message fields
    };


    // --------------------------------------------------------------------------------------------


    /*!
        \brief A model for displaying all available signal spies.

        Provides functions for creating and destroying SignalSpy objects. In addition all created
        spies will share the same SignalLogger, thus providing consistent logging across all spies.
    */
    class SignalSpyModel : public QAbstractTableModel
    {
    public:
        typedef enum COLUMNS {
            kSignature,
            kObject,
            kAddress,
            kSuperClass,
            kColumnCount
        } Columns;

    public:
        SignalSpyModel (QObject* inParent = 0);
        virtual ~SignalSpyModel ();
        
        bool ContainsSignalSpy (const QObject* inObject, const QString& inSignal) const;
        void CreateSignalSpy (const QObject* inObject, const QString& inSignal);
        void DestroySignalSpy (const QObject* inObject, const QString& inSignal);

        void SetLogOptions (bool inShowTimestamp, bool inShowObject, bool inShowAddress,
                            bool inShowSignature, bool inShowEmitCount, bool inShowArguments,
                            bool inPrettyFormatting, QChar inSeparator);
        QString GetExampleLog () const;

        // qt overrides
        virtual int rowCount (const QModelIndex& inParent = QModelIndex ()) const;
        virtual int columnCount (const QModelIndex& inParent = QModelIndex ()) const;
        virtual Qt::ItemFlags flags (const QModelIndex& inIndex) const;
        virtual QVariant data (const QModelIndex& inIndex, int inRole) const;
        virtual QVariant headerData (int inSection, Qt::Orientation inOrientation, int inRole = Qt::DisplayRole) const;
        virtual bool removeRows (int inRow, int inCount, const QModelIndex& inParent = QModelIndex ());

    private:
        std::vector <SignalSpy*> mSignalSpies;  //!< Contains all created signal spies
        SignalLogger mLogger;                   //!< Used by all signal spies for consistent log formatting
    };
}

#endif //_SIGNALSPY__31_03_09__01_53_37__H_