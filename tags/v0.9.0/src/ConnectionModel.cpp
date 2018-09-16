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
    \brief Contains ConnectionModel related definitions
*/


#include "ConnectionModel.h"
#include <algorithm>
#include <algorithm>
#include <QApplication>
#include <QBrush>
#include <QFont>


namespace conan {
    MethodData::MethodData () :
        mConnectionType (Qt::AutoConnection),
        mAccess (QMetaMethod::Public)
    {}

    bool MethodData::operator!= (const MethodData& inSource) const {
        return mSignature != inSource.mSignature ||
               mObject != inSource.mObject ||
               mAddress != inSource.mAddress ||
               mSuperClass != inSource.mSuperClass ||
               mConnectionType != inSource.mConnectionType ||
               mAccess != inSource.mAccess ||
               mMethodType != inSource.mMethodType;
    }

    bool MethodData::operator== (const MethodData& inSource) const {
        return !(*this != inSource);
    }


    // ------------------------------------------------------------------------------------------------


    ConnectionModel::ConnectionModel (QObject* inParent) : QAbstractItemModel (inParent) {
        setObjectName ("ConnectionModel");
    }

    /*!
        \brief Resets the model with new source data.
    */
    void ConnectionModel::SetData (const std::vector <ConnectionData>& inConnections) {
        mConnections = inConnections;
        reset ();
    }


    /*!
        \brief Returns the number of rows under the given parent.
    */
    int ConnectionModel::rowCount (const QModelIndex& inParent) const {
        if (!inParent.isValid ()) {
            return static_cast <int> (mConnections.size ());
        }
        else if (const ConnectionData* connectionData = GetConnectionData (inParent)) {
            return static_cast <int> (connectionData->second.size ());
        }
        return 0;
    }


    /*!
        \brief Returns the number of columns of the model.
    */
    int ConnectionModel::columnCount (const QModelIndex& /*inParent*/) const {
        return kColumnCount;
    }

    /*!
        \brief Returns the item flags for the given index.
    */
    Qt::ItemFlags ConnectionModel::flags (const QModelIndex& inIndex) const {
        Qt::ItemFlags flags = Qt::ItemIsEnabled;
        // the access column only contains icons and is not selectable
        if (inIndex.column () != kAccess) flags |= Qt::ItemIsSelectable;
        return flags;
    }

    /*!
        \brief Returns the data stored under the given role for the item referred to by the index.
    */
    QVariant ConnectionModel::data (const QModelIndex& inIndex, int inRole) const {
        if (!inIndex.isValid () || mConnections.empty ()) {
            return QVariant ();
        }
        int row = inIndex.row ();
        int column = inIndex.column ();
        if (inRole == Qt::DisplayRole) {
            if (column == kSignature) {
                if (const MethodData* methodData = GetMethodData (inIndex)) {
                    return methodData->mSignature;
                }
                else if (const ConnectionData* connectionData = GetConnectionData (inIndex)) {
                    return connectionData->first.mSignature;
                }            
            }
            else if (column == kObject) {
                if (const MethodData* methodData = GetMethodData (inIndex)) {
                    return methodData->mObject;
                }
            }
            else if (column == kAddress) {
                if (const MethodData* methodData = GetMethodData (inIndex)) {
                    return methodData->mAddress;
                }
            }
            else if (column == kSuperClass) {
                if (const ConnectionData* connectionData = GetConnectionData (inIndex)) {
                    return connectionData->first.mSuperClass;
                }
            }
            else if (column == kConnectionType) {
                if (const MethodData* methodData = GetMethodData (inIndex)) {
                    if (methodData->mConnectionType == Qt::AutoConnection) {
                        return QString ("Auto");
                    }
                    if (methodData->mConnectionType == Qt::DirectConnection) {
                        return QString ("Direct");
                    }
                    if (methodData->mConnectionType == Qt::QueuedConnection) {
                        return QString ("Queued");
                    }
                    if (methodData->mConnectionType == Qt::BlockingQueuedConnection) {
                        return QString ("Blocking");
                    }
                }
            }
        }
        else if (inRole == Qt::BackgroundRole) {
            if (column != kAccess) {
                if (const MethodData* methodData = GetMethodData (inIndex)) {
                    if (const ConnectionData* connectionData = GetConnectionData (inIndex.parent ())) {
                        // count number of duplicates in parent
                        if (1 < std::count_if (connectionData->second.begin (), connectionData->second.end (), std::bind2nd (std::equal_to <MethodData> (), *methodData))) {
                            return QBrush (Qt::yellow);
                        }
                    }
                }
            }
        }
        else if (inRole == Qt::ToolTipRole) {
            QMetaMethod::Access access = QMetaMethod::Public;
            if (const MethodData* methodData = GetMethodData (inIndex)) {
                if (const ConnectionData* connectionData = GetConnectionData (inIndex.parent ())) {
                    // count number of duplicates in parent
                    if (1 < std::count_if (connectionData->second.begin (), connectionData->second.end (), std::bind2nd (std::equal_to <MethodData> (), *methodData))) {
                        return QString ("Duplicate connection");
                    }
                    access = methodData->mAccess;
                }
            }
            else if (const ConnectionData* connectionData = GetConnectionData (inIndex)) {
                access = connectionData->first.mAccess;
            }
            if (column == kAccess) {
                if (access == QMetaMethod::Protected) {
                    return QString ("Protected method");
                }
                else if (access == QMetaMethod::Private) {
                    return QString ("Private method");
                }
                else {
                    return QString ("Public method");
                }
            }
        }
        else if (inRole == Qt::DecorationRole) {
            if (column == kAccess) {
                QMetaMethod::Access access = QMetaMethod::Public;
                if (const ConnectionData* connectionData = GetConnectionData (inIndex)) {
                    access = connectionData->first.mAccess;
                }
                if (access == QMetaMethod::Protected) {
                    return QPixmap (":/icons/conan/key");
                }
                else if (access == QMetaMethod::Private) {
                    return QPixmap (":/icons/conan/lock");
                }
            }
        }
        return QVariant ();
    }

    /*!
        \brief Returns the data for the given role and section in the header with the specified orientation.
    */
    QVariant ConnectionModel::headerData (int inSection, Qt::Orientation inOrientation, int inRole) const {
        if (inRole != Qt::DisplayRole) {
            return QVariant ();
        }
        if (inOrientation == Qt::Horizontal) {
            if (inSection == kSignature) {
                return QString ("Signature");
            }
            else if (inSection == kObject) {
                return QString ("Object");
            }
            else if (inSection == kAddress) {
                return QString ("Address");
            }
            else if (inSection == kSuperClass) {
                return QString ("Declared in");
            }
            else if (inSection == kConnectionType) {
                return QString ("Connection");
            }
            else if (inSection == kAccess) {
                return QString ("");
            }
        }
        return QVariant();
    }

    /*!
        \brief Returns the index of the item in the model specified by the given row, column and parent index.

        The internal pointer of an index is set to zero for indices that refer to top level signals/slots.
        For connected methods the internal pointer is set to the ConnectionData that contains them.
    */
    QModelIndex ConnectionModel::index (int inRow, int inColumn, const QModelIndex& inParent) const {
        if (hasIndex (inRow, inColumn, inParent)) {
            if (!inParent.isValid ()) {
                return createIndex (inRow, inColumn, 0);
            }
            else {
                return createIndex (inRow, inColumn, const_cast <ConnectionData*> (&mConnections [inParent.row ()]));
            }
        }
        return QModelIndex ();
    }

    /*!
        \brief Returns the parent of the model item with the given index, or QModelIndex() if it has no parent.

        Only items referring to connected methods have a parent.
    */
    QModelIndex ConnectionModel::parent (const QModelIndex& inIndex) const {
        if (!inIndex.internalPointer ()) {    // connection data
            return QModelIndex ();
        }
        int row = Row (reinterpret_cast <const ConnectionData*> (inIndex.internalPointer ()));
        return createIndex (row, 0, 0);
    }

    /*!
        \brief Retrieves the ConnectionData associated with the given index.
        \param[in] inIndex  The model index
        \return             The corresponding ConnectionData when \p index has no parent; otherwise 0
    */
    const ConnectionData* ConnectionModel::GetConnectionData (const QModelIndex& inIndex) const {
        if (inIndex.isValid () && !inIndex.parent ().isValid ()) {
            return reinterpret_cast <const ConnectionData*> (&mConnections [inIndex.row ()]);
        }
        return 0;
    }

    /*!
        \brief Retrieves the MethodData associated with the given index.
        \param[in] inIndex  The model index
        \return             The corresponding MethodData when \p index has a valid parent; otherwise 0
    */
    const MethodData* ConnectionModel::GetMethodData (const QModelIndex& inIndex) const {
        if (const ConnectionData* connectionData = reinterpret_cast <const ConnectionData*> (inIndex.internalPointer ())) {
            return &connectionData->second [inIndex.row ()];
        }
        return 0;
    }

    /*!
        \brief Returns the row of the signal/slot that corresponds to the given ConnectionData
        \param[in] inConnectionData     The connection data
        \return                         The row of the signal/slot that corresponds to the connection data; otherwise -1
    */
    int ConnectionModel::Row (const ConnectionData* inConnectionData) const {
        if (inConnectionData) {
            for (size_t i=0; i<mConnections.size (); i++) {
                if (&mConnections [i] == inConnectionData) {
                    return static_cast <int> (i);
                }
            }
        }
        return -1;
    }


    // ------------------------------------------------------------------------------------------------


    ConnectionFilterProxyModel::ConnectionFilterProxyModel (QObject* inParent) :
        QSortFilterProxyModel (inParent), mInactiveFiltering (false), mInheritedFiltering (false)
    {
    }


    /*!
        \brief Sets the class name used during 'inherited method' filtering.
    */
    void ConnectionFilterProxyModel::SetClassName (const QString& inClassName) {
        mClassName = inClassName;
        invalidateFilter ();
    }

    /*!
        \brief Returns true if the item indicated by the given source row and parent should be included in the model; otherwise returns false.
    */
    bool ConnectionFilterProxyModel::filterAcceptsRow (int inSourceRow, const QModelIndex &inSourceParent) const {
        if (const ConnectionModel* model = dynamic_cast <ConnectionModel*> (sourceModel ())) {
            QModelIndex index = model->index (inSourceRow, 0, inSourceParent);
            if (model->GetMethodData (index)) {
                // never filter methods
                return true;
            }    
            if (mInactiveFiltering && !model->rowCount (index)) {
                // filter connections without (connected) methods
                return false;
            }
            if (mInheritedFiltering) {
                QModelIndex superClassIndex = model->index (inSourceRow, ConnectionModel::kSuperClass, inSourceParent);
                if (model->data (superClassIndex, Qt::DisplayRole).toString () != mClassName) {
                    // filter connections declared in wrong classes
                    return false;
                }
            }
        }
        return true;
    }


    /*!
        \brief Returns true if the value of the item referred to by the left index is less than that of the right index, otherwise returns false.

        The column sort order is always: the primary (selected) sort column, signature column, object column, address column.
    */
    bool ConnectionFilterProxyModel::lessThan (const QModelIndex& inLeft, const QModelIndex& inRight) const {
        if (inLeft.column () == ConnectionModel::kAccess) {
            // sortorder: access, signature, object, address
            if (const ConnectionModel* model = dynamic_cast <ConnectionModel*> (sourceModel ())) {
                QMetaMethod::Access leftAccess = QMetaMethod::Public;
                QMetaMethod::Access rightAccess = QMetaMethod::Public;
                if (const ConnectionData* leftConnection = model->GetConnectionData (inLeft)) {
                    leftAccess = leftConnection->first.mAccess;
                }
                else if (const MethodData* leftMethod = model->GetMethodData (inLeft)) {
                    leftAccess = leftMethod->mAccess;
                }
                if (const ConnectionData* rightConnection = model->GetConnectionData (inRight)) {
                    rightAccess = rightConnection->first.mAccess;
                }
                else if (const MethodData* rightMethod = model->GetMethodData (inRight)) {
                    rightAccess = rightMethod->mAccess;
                }
                if (leftAccess == rightAccess) {
                    QList <int> sortOrder;
                    sortOrder << ConnectionModel::kSignature << ConnectionModel::kObject << ConnectionModel::kAddress;
                    return lessThan (inLeft, inRight, sortOrder);
                }
                else {
                    // access values: private < protected < public
                    return leftAccess > rightAccess;
                }
            }
        }
        else if (inLeft.column () == ConnectionModel::kAddress) {
            // sortorder: address, signature, object
            QList <int> sortOrder;
            sortOrder << ConnectionModel::kAddress << ConnectionModel::kSignature << ConnectionModel::kObject;
            return lessThan (inLeft, inRight, sortOrder);
        }
        else if (inLeft.column () == ConnectionModel::kConnectionType) {
            // sortorder: connection type, signature, object, address
            QList <int> sortOrder;
            sortOrder << ConnectionModel::kConnectionType << ConnectionModel::kSignature << ConnectionModel::kObject << ConnectionModel::kAddress;
            return lessThan (inLeft, inRight, sortOrder);
        }
        else if (inLeft.column () == ConnectionModel::kObject) {
            // sortorder: object, signature, address
            QList <int> sortOrder;
            sortOrder << ConnectionModel::kObject << ConnectionModel::kSignature << ConnectionModel::kAddress;
            return lessThan (inLeft, inRight, sortOrder);
        }
        else if (inLeft.column () == ConnectionModel::kSignature) {
            // sortorder: signature, object, address
            QList <int> sortOrder;
            sortOrder << ConnectionModel::kSignature << ConnectionModel::kObject << ConnectionModel::kAddress;
            return lessThan (inLeft, inRight, sortOrder);
        }
        else if (inLeft.column () == ConnectionModel::kSuperClass) {
            // sortorder: superClass, signature, object, address
            QList <int> sortOrder;
            sortOrder << ConnectionModel::kSuperClass << ConnectionModel::kSignature << ConnectionModel::kObject << ConnectionModel::kAddress;
            return lessThan (inLeft, inRight, sortOrder);
        }
        return false;
    }


    /*!
        \brief Performs the actual value based index sorting using the given sort column order.
    */
    bool ConnectionFilterProxyModel::lessThan (const QModelIndex& inLeft, const QModelIndex& inRight, const QList <int>& inSortOrder) const {
        foreach (int column, inSortOrder) {
            QString leftData = sourceModel ()->data (inLeft.sibling (inLeft.row (), column)).toString ();
            QString rightData = sourceModel ()->data (inRight.sibling (inRight.row (), column)).toString ();
            if (leftData == rightData) {
                continue;
            }
            return leftData < rightData;
        }
        return false;
    }

    /*!
        Controls inactive method filtering
    */
    void ConnectionFilterProxyModel::SlotEnableInactiveFiltering (bool inEnable) {
        mInactiveFiltering = inEnable;
        invalidateFilter ();
    }

    /*!
        Controls inherited method filtering
    */
    void ConnectionFilterProxyModel::SlotEnableInheritedFiltering (bool inEnable) {
        mInheritedFiltering = inEnable;
        invalidateFilter ();
    }
} // namespace conan