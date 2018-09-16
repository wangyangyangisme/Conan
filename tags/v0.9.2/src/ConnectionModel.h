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
    \brief Contains ConnectionModel related declarations
*/


#ifndef _CONNECTIONMODEL__10_12_08__01_16_49__H_
#define _CONNECTIONMODEL__10_12_08__01_16_49__H_


#include <QAbstractItemModel>
#include <QMetaMethod>
#include <QSortFilterProxyModel>
#include <vector>


namespace conan {
    //! \brief Represents a single signal or slot
    struct MethodData {
        MethodData ();

        bool operator!= (const MethodData& inSource) const;
        bool operator== (const MethodData& inSource) const;

        QString mSignature;                     //!< signal or slot method signature
        QString mObject;                        //!< className::objectName
        QString mAddress;                       //!< object address
        QString mSuperClass;                    //!< class that declared the method
        uint mConnectionType;                   //!< auto, direct, queued, blocking
        QMetaMethod::Access mAccess;            //!< public, protected, private
        QMetaMethod::MethodType mMethodType;    //!< signal, slot
    };

    // ------------------------------------------------------------------------------------------------

    //! \brief Represents a signal or slot and all connected methods
    struct ConnectionData {
        ConnectionData (const MethodData& inMethod);

        bool ContainsDuplicateConnections () const;
        int CountConnections (const MethodData& inMethod) const;

        MethodData mMethod;                     //!< Signal or slot data
        std::vector <MethodData> mConnections;  //!< All connected methods
    };


    // ------------------------------------------------------------------------------------------------


    /*!
        \brief A hierarchical read-only model for a list of ConnectionData.

        Note that duplicate connections are marked in yellow.
    */
    class ConnectionModel : public QAbstractItemModel
    {
    public:
        //! ConnectionModel columns
        typedef enum COLUMNS {
            kSignature,
            kObject,
            kAddress,
            kConnectionType,
            kSuperClass,
            kAccess,
            kColumnCount
        } Columns;

    public:
        ConnectionModel (QObject* inParent = 0);

        void SetData (const std::vector <ConnectionData>& inConnections);

        // qt overrides
        virtual int rowCount (const QModelIndex& inParent = QModelIndex ()) const;
        virtual int columnCount (const QModelIndex& inParent = QModelIndex ()) const;
        virtual Qt::ItemFlags flags (const QModelIndex& inIndex) const;
        virtual QVariant data (const QModelIndex& inIndex, int inRole) const;
        virtual QVariant headerData (int inSection, Qt::Orientation inOrientation, int inRole = Qt::DisplayRole) const;
        virtual QModelIndex index (int inRow, int inColumn, const QModelIndex& inParent = QModelIndex ()) const;
        virtual QModelIndex parent (const QModelIndex& inIndex) const;

        const MethodData* GetMethodData (const QModelIndex& inIndex) const;
        const ConnectionData* GetConnectionData (const QModelIndex& inIndex) const;

        bool ContainsDuplicateConnections (const QModelIndex& inIndex) const;

    private:
        int Row (const ConnectionData* inConnectionData) const;

    private:
        std::vector <ConnectionData> mConnections; //!< The source data of the model
    };


    // ------------------------------------------------------------------------------------------------


    /*!
        \brief Provides sorting and filtering functionality specific to ConnectionModels

        Two filtering options are provided: inactive methods and inherited methods.
        Inactive methods are signals or slots without any connections.
        Inherited methods are signals or slots that are declared in a base class other then \p mClassName.
    */
    class ConnectionFilterProxyModel : public QSortFilterProxyModel {
        Q_OBJECT

    public:
        ConnectionFilterProxyModel (QObject* inParent = 0);
        void SetClassName (const QString& inClassName);

    protected:
        // qt overrides
        virtual bool filterAcceptsRow (int inSourceRow, const QModelIndex &inSourceParent) const;
        virtual bool lessThan (const QModelIndex& inLeft, const QModelIndex& inRight) const;
        virtual bool lessThan (const QModelIndex& inLeft, const QModelIndex& inRight, const QList <int>& inSortOrder) const;

    public slots:
        void SlotEnableInactiveFiltering (bool inEnable);
        void SlotEnableInheritedFiltering (bool inEnable);

    private:
        bool mInactiveFiltering;    //!< Controls inactive method filtering
        bool mInheritedFiltering;   //!< Controls inherited method filtering
        QString mClassName;         //!< The class name of the object whose connections are used by the ConnectionModel
    };
} // namespace conan


#endif //_CONNECTIONMODEL__10_12_08__01_16_49__H_