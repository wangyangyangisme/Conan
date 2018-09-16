/*!
    Conan - Connection Analyzer for Qt
    Copyright (C) 2008 - 2011 Elmar de Koning, edekoning@gmail.com

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
    \brief Contains all code that uses the (version specific) private API of Qt
*/


#ifndef _CONANWIDGET_P__09_12_08__12_00_55__H_
#define _CONANWIDGET_P__09_12_08__12_00_55__H_


#include "ConnectionModel.h"
#include "ObjectUtility.h"
#include <QtCore/QMetaObject>
#include <QtCore/QtDebug>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QVector>

// private Qt headers
#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>


namespace conan {

    //! Contains helper methods
    namespace /*unnamed*/ {

        /*!
            \brief For a given metaobject, compute the signal offset, and the method offset (including signals)

            This function was copied directly from qobject.cpp
        */
        void computeOffsets(const QMetaObject *metaobject, int *signalOffset, int *methodOffset) {
            *signalOffset = *methodOffset = 0;
            const QMetaObject *m = metaobject->d.superdata;
            while (m) {
                const QMetaObjectPrivate *d = QMetaObjectPrivate::get(m);
                *methodOffset += d->methodCount;
                *signalOffset += (d->revision >= 4) ? d->signalCount : d->methodCount;
                /*Before Qt 4.6 (revision 4), the signalCount information was not generated by moc.
                so for compatibility we consider all the method as slot for old moc output*/
                m = m->d.superdata;
            }
        }

        /*!
            \brief Compute the signal index offset

            This function was copied directly from QObject::dumpObjectInfo() in qobject.cpp
        */
        void computeOffsets (const QObject* inObject, int& signal_index, int& offset, int& offsetToNextMetaObject) {
            if (signal_index >= offsetToNextMetaObject) {
                const QMetaObject *mo = inObject->metaObject();
                int signalOffset, methodOffset;
                computeOffsets(mo, &signalOffset, &methodOffset);
                while (signalOffset > signal_index) {
                    mo = mo->superClass();
                    offsetToNextMetaObject = signalOffset;
                    computeOffsets(mo, &signalOffset, &methodOffset);
                }
                offset = methodOffset - signalOffset;
            }
        }
    }


    // --------------------------------------------------------------------------------------------


    /*!
        \brief Qt version specific code

        Contains functions that use the private API of Qt to access information about an object's
        signals, slots and active connections. Note that since Qt 4.6.0, it is required to lock a
        specific mutex before accessing the connection data of an object using:
        QMutexLocker locker(signalSlotLock(const QObject*));
        Unfortunately the signalSlotLock method and the mutex it returns are all defined in
        qobject.cpp at file level scope and are thus unaccessible. As a result these methods and
        Conan in general are not thread safe. To be fair, this is not such a big deal during normal
        use. Connections are usually made during or after Object construction, and for the large
        part this is done on the gui thread (where Conan lives).
    */
    namespace priv {

        /*!
            \brief Builds a list of all signals and their connections for the given object.
            \param[in]  inObject        The object
            \param[out] outConnections  Contains all the object's signals and their connections
        */
        void BuildSignalData (const QObject* inObject, QVector <ConnectionData>& outConnections) {
            outConnections.clear ();
            if (!inObject) {
                return;
            }
            const QMetaObject* metaObject = inObject->metaObject ();
            if (!metaObject) {
                return;
            }
            // map <signal signature, outConnections index>
            QMap <QString, int> signalIndices;
            QSet <QString> slotSignatures;
            int signalIndex = 0;
            // first gather all signals
            while (metaObject) {
                int offset = metaObject->methodOffset ();
                int count = metaObject->methodCount ();
                // only process those methods that are unique to the current metaObject
                for (int m=count-1; m>=offset; m--) {
                    QMetaMethod method = metaObject->method (m);
                    if (method.methodType () == QMetaMethod::Slot) {
                        slotSignatures.insert (method.signature ());
                    }
                    else if (method.methodType () == QMetaMethod::Signal) {
                        MethodData methodData;
                        methodData.mAccess = method.access ();
                        methodData.mAddress = ObjectUtility::Address (inObject);
                        methodData.mObject = const_cast <QObject*> (inObject);
                        methodData.mName = ObjectUtility::Name (inObject);
                        methodData.mClass = ObjectUtility::Class (inObject);
                        methodData.mSignature = method.signature ();
                        methodData.mSuperClass = metaObject->className ();
                        methodData.mMethodType = method.methodType ();
                        // store
                        outConnections.push_back (ConnectionData (methodData));
                        signalIndices [method.signature ()] = signalIndex++;
                    }
                }
                metaObject = metaObject->superClass ();
            }

            try {
                if (QObjectPrivate* object_p = QObjectPrivate::get (const_cast <QObject*> (inObject))) {

                    if (!object_p->connectionLists)
                        return;

                    int offset = 0;
                    int offsetToNextMetaObject = 0;
                    // QObjectConnectionListVector is declared in qobject.cpp,
                    // luckily it derives from QVector <QObjectPrivate::ConnectionList>
                    QVector <QObjectPrivate::ConnectionList>* connectionLists =
                        reinterpret_cast <QVector <QObjectPrivate::ConnectionList>*> (object_p->connectionLists);

                    // look for connections where this object is the sender
                    for (int signal_index = 0; signal_index < connectionLists->count (); ++signal_index) {
                        computeOffsets (inObject, signal_index, offset, offsetToNextMetaObject);

                        const QMetaMethod signal = inObject->metaObject ()->method (signal_index + offset);
                        // find the connection index of the current signal
                        if (signalIndices.find (signal.signature ()) == signalIndices.end ()) {
                            // its probably a slot signature (dubious)
                            if (slotSignatures.find (signal.signature ()) == slotSignatures.end ()) {
                                // this should never happen
                                qDebug () << "Found a signal with signature '" << signal.signature () << "' that could not be matched to any QMetaMethod.";
                                Q_ASSERT (false);
                            }
                            continue;
                        }
                        int connectionIndex = signalIndices [signal.signature ()];
                        // find all receivers of the current signal
                        const QObjectPrivate::Connection *c = connectionLists->at (signal_index).first;
                        while (c) {
                            if (!c->receiver) {
                                // disconnected receiver
                                c = c->nextConnectionList;
                                continue;
                            }
                            const QMetaObject *receiverMetaObject = c->receiver->metaObject ();
                            const QMetaMethod method = receiverMetaObject->method (c->method);

                            if (!method.signature ()) {
                                // this only occurs when c.receiver has not been moc'ed
                                // i.e. SignalSpy or QSignalSpy
                                c = c->nextConnectionList;
                                continue;
                            }

                            MethodData slotData;
                            slotData.mAccess = method.access ();
                            slotData.mAddress = ObjectUtility::Address (c->receiver);
                            slotData.mConnectionType = c->connectionType;
                            slotData.mObject = c->receiver;
                            slotData.mName = ObjectUtility::Name (c->receiver);
                            slotData.mClass = ObjectUtility::Class (c->receiver);
                            slotData.mSignature = method.signature ();
                            slotData.mMethodType = method.methodType ();
                            // locate the class from which the method originates
                            int index = receiverMetaObject->indexOfMethod (method.signature ());
                            while (receiverMetaObject) {
                                int offset = receiverMetaObject->methodOffset ();
                                int count = receiverMetaObject->methodCount ();
                                if (offset <= index && index < count) {
                                    slotData.mSuperClass = receiverMetaObject->className ();
                                    break;
                                }
                                receiverMetaObject = receiverMetaObject->superClass ();
                            }
                            // store
                            outConnections [connectionIndex].mConnections.push_back (slotData);

                            c = c->nextConnectionList;
                        }
                    }
                }
            }
            catch (...) {
                qDebug () << "Unable to process signal connections for object" << ObjectUtility::QualifiedName (inObject) << ObjectUtility::Address (inObject);
            }
        }

        /*!
            \brief Builds a list of all slots and their connections for the given object.
            \param[in]  inObject        The object
            \param[out] outConnections  Contains all the object's slots and their connections
        */
        void BuildSlotData (const QObject* inObject, QVector <ConnectionData>& outConnections) {
            outConnections.clear ();
            if (!inObject) {
                return;
            }
            const QMetaObject* metaObject = inObject->metaObject ();
            if (!metaObject) {
                return;
            }
            // map <signature, index>
            QMap <QString, int> slotIndices;
            QSet <QString> signalSignatures;
            int slotIndex = 0;
            // first gather all slots
            while (metaObject) {
                int offset = metaObject->methodOffset ();
                int count = metaObject->methodCount ();
                // only process those methods that are unique to the current metaObject
                for (int m=count-1; m>=offset; m--) {
                    QMetaMethod method = metaObject->method (m);
                    if (method.methodType () == QMetaMethod::Signal) {
                        signalSignatures.insert (method.signature ());
                    }
                    else if (method.methodType () == QMetaMethod::Slot) {
                        MethodData methodData;
                        methodData.mAccess = method.access ();
                        methodData.mAddress = ObjectUtility::Address (inObject);
                        methodData.mObject = const_cast <QObject*> (inObject);
                        methodData.mName = ObjectUtility::Name (inObject);
                        methodData.mClass = ObjectUtility::Class (inObject);
                        methodData.mSignature = method.signature ();
                        methodData.mSuperClass = metaObject->className ();
                        methodData.mMethodType = method.methodType ();
                        // store
                        outConnections.push_back (ConnectionData (methodData));
                        slotIndices [method.signature ()] = slotIndex++;
                    }
                }
                metaObject = metaObject->superClass ();
            }

            try {
                if (QObjectPrivate* object_p = QObjectPrivate::get (const_cast <QObject*> (inObject))) {

                    if (!object_p->senders)
                        return;

                    QSet <QPair <QObject*, int> > processedConnections;  // set <sender, slotIndex>
                    
                    // process all incomming signals
                    for (QObjectPrivate::Connection *s = object_p->senders; s; s = s->next) {
                        const QMetaMethod slot = inObject->metaObject ()->method (s->method);

                        // locate existing connection data for the slot
                        if (slotIndices.find (slot.signature ()) == slotIndices.end ()) {
                            // its probably a signal->signal connection
                            if (signalSignatures.find (slot.signature ()) == signalSignatures.end ()) {
                                // this should never happen
                                qDebug () << "Found a slot with signature '" << slot.signature () << "' that could not be matched to any QMetaMethod.";
                                Q_ASSERT (false);
                            }
                            continue;
                        }
                        int connectionIndex = slotIndices [slot.signature ()];

                        if (processedConnections.contains (qMakePair (s->sender, connectionIndex)))
                            // we already processed all connections for this sender to the slot
                            continue;
                        processedConnections.insert (qMakePair (s->sender, connectionIndex));

                        // build the senders signal data
                        QVector <ConnectionData> senderConnections;
                        BuildSignalData (s->sender, senderConnections);

                        // process all senders signals
                        for (int c = 0; c < senderConnections.size (); c++) {
                            const ConnectionData& connectionData = senderConnections [c];
                            // process all slots of the current signal
                            for (int s = 0; s < connectionData.mConnections.size (); s++) {
                                MethodData connectedSlot = connectionData.mConnections[s];
                                // does it match the current slot
                                if (connectedSlot.mObject == inObject && connectedSlot.mSignature == slot.signature ()) {
                                    MethodData signalData (connectionData.mMethod);
                                    signalData.mConnectionType = connectedSlot.mConnectionType;
                                    // store
                                    outConnections [connectionIndex].mConnections.push_back (signalData);
                                }
                                // continue, because there might be duplicate connections
                            }
                        }
                    }
                }
            }
            catch (...) {
                qDebug () << "Unable to process slot connections for object" << ObjectUtility::QualifiedName (inObject) << ObjectUtility::Address (inObject);
            }
        }
    } // namespace priv
} // namespace conan

#endif //_CONANWIDGET_P__09_12_08__12_00_55__H_