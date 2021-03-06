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
    \brief Contains all code that uses the (version specific) private API of Qt
*/


#ifndef _ConanWidget_P__09_12_08__12_00_55__H_
#define _ConanWidget_P__09_12_08__12_00_55__H_


#include "ConnectionModel.h"
#include "ObjectUtility.h"
#include <assert.h>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <set>
#include <vector>
// private Qt headers
#include <corelib/kernel/qobject_p.h>    // needed to be able to retrieve an object's active connections
#include <corelib/thread/qthread_p.h>    // needed to be able to lock an object


namespace conan {

    /*!
        \brief Qt version specific code

        Contains functions that use the private API of Qt to access information about an object's
        signals, slots and active connections. Before any of those functions are called the object
        must be locked! A convenience class, the ObjectLocker, is provided to achieve this.
    */
    namespace priv {

        /*!
            \brief Locks the mutex of a given object upon creation and unlocks it upon destruction.
        */
        struct ObjectLocker {
            ObjectLocker (const QObject* inObject) :
                mLocker (inObject ? &QObjectPrivate::get (const_cast <QObject*> (inObject))->threadData->mutex : 0)
            {}
            ~ObjectLocker () {
                mLocker.unlock ();
            }
        private:
            QMutexLocker mLocker;
        };

        /*!
            \brief Builds a list of all signals and their connections for the given object.
            \param[in]  inObject        The object
            \param[out] outConnections  Contains all the object's signals and their connections
        */
        void BuildSignalData (const QObject* inObject, std::vector <ConnectionData>& outConnections) {
            outConnections.clear ();
            if (!inObject) {
                return;
            }
            const QMetaObject* metaObject = inObject->metaObject ();
            if (!metaObject) {
                return;
            }
            // map <signal signature, outConnections index>
            std::map <std::string, int> signalIndices;
            std::set <QString> slotSignatures;
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
            if (QObjectPrivate* object_p = QObjectPrivate::get (const_cast <QObject*> (inObject))) {
                // look for connections where this object is the sender
                if (object_p->connectionLists) {
                    // QObjectConnectionListVector is declared in qobject.cpp,
                    // luckily it derives from QVector <QObjectPrivate::ConnectionList>
                    QVector <QObjectPrivate::ConnectionList>* connectionLists =
                        reinterpret_cast <QVector <QObjectPrivate::ConnectionList>*> (object_p->connectionLists);

                    for (int signal_index = 0; signal_index < connectionLists->count (); ++signal_index) {
                        const QMetaMethod signal = inObject->metaObject ()->method (signal_index);
                        // find the connection index of the current signal
                        if (signalIndices.find (signal.signature ()) == signalIndices.end ()) {
                            // its probably a slot signature
                            if (slotSignatures.find (signal.signature ()) == slotSignatures.end ()) {
                                // this should never happen
                                assert (false);
                            }
                            continue;
                        }
                        int connectionIndex = signalIndices [signal.signature ()];
                        // find all receivers of the current signal
                        const QObjectPrivate::ConnectionList &connectionList = connectionLists->at (signal_index);
                        for (int i = 0; i < connectionList.count (); ++i) {
                            const QObjectPrivate::Connection &c = connectionList.at (i);
                            if (!c.receiver) {
                                continue;
                            }
                            const QMetaObject *receiverMetaObject = c.receiver->metaObject ();
                            const QMetaMethod method = receiverMetaObject->method (c.method);

                            if (!method.signature ()) {
                                // this only occurs when c.receiver has not been moc'ed
                                // i.e. SignalSpy or QSignalSpy
                                continue;
                            }

                            MethodData slotData;
                            slotData.mAccess = method.access ();
                            slotData.mAddress = ObjectUtility::Address (c.receiver);
                            slotData.mConnectionType = c.connectionType;
                            slotData.mObject = c.receiver;
                            slotData.mName = ObjectUtility::Name (c.receiver);
                            slotData.mClass = ObjectUtility::Class (c.receiver);
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
                        }
                    }
                }
            }
        }

        /*!
            \brief Builds a list of all slots and their connections for the given object.
            \param[in]  inObject        The object
            \param[out] outConnections  Contains all the object's slots and their connections
        */
        void BuildSlotData (const QObject* inObject, std::vector <ConnectionData>& outConnections) {
            outConnections.clear ();
            if (!inObject) {
                return;
            }
            const QMetaObject* metaObject = inObject->metaObject ();
            if (!metaObject) {
                return;
            }
            // map <signature, index>
            std::map <QString, int> slotIndices;
            std::set <QString> signalSignatures;
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
            QString address = ObjectUtility::Address (inObject);

            if (QObjectPrivate* object_p = QObjectPrivate::get (const_cast <QObject*> (inObject))) {
                if (!object_p->senders.isEmpty ()) {
                    // process all incomming signals
                    for (int i = 0; i < object_p->senders.count (); ++i) {
                        const QObjectPrivate::Sender &s = object_p->senders.at (i);
                        const QMetaObject *senderMetaObject = s.sender->metaObject ();
                        const QMetaMethod signal = senderMetaObject->method (s.signal);
                        // duplicate signals cannot be detected using only the sender list,
                        // instead we use the complete signal data of a signal's sender
                        std::vector <ConnectionData> senderConnections;
                        BuildSignalData (s.sender, senderConnections);
                        // find the slots connected to the signal using the sender's connection data
                        for (size_t c = 0; c < senderConnections.size (); c++) {
                            const ConnectionData& connectionData = senderConnections [c];
                            // locate the signal
                            if (connectionData.mMethod.mSignature == signal.signature ()) {
                                for (size_t m = 0; m < connectionData.mConnections.size (); m++) {
                                    const MethodData& methodData = connectionData.mConnections [m];
                                    // locate the slot
                                    if (methodData.mAddress == address) {
                                        // locate existing connection data for the slot
                                        if (slotIndices.find (methodData.mSignature) == slotIndices.end ()) {
                                            // its probably a signal->signal connection
                                            if (signalSignatures.find (methodData.mSignature) == signalSignatures.end ()) {
                                                // this should never happen
                                                assert (false);
                                            }
                                            continue;
                                        }
                                        int slotIndex = slotIndices [methodData.mSignature];
                                        // store
                                        outConnections [slotIndex].mConnections.push_back (connectionData.mMethod);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /*!
            \brief Returns a list of all base classes upto QObject for the given object.
            \param[in]  inObject    The object
            \param[out] outList     The list of base classes
        */
        void BuildInheritanceData (const QObject* inObject, QStringList& outList) {
            if (!inObject) {
                return;
            }
            const QMetaObject* metaObject = inObject->metaObject ();
            while (metaObject) {
                outList.append (metaObject->className ());
                metaObject = metaObject->superClass ();
            }
        }
    } // namespace priv
} // namespace conan

#endif //_ConanWidget_P__09_12_08__12_00_55__H_
