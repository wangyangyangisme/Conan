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
    \brief Contains definitions for various test classes
*/


#include "TestClasses.h"


namespace conan {
    namespace test {

        Hierarchy::Hierarchy(const QString& inName, QObject* inParent) :
            QObject (inParent)
        {
            setObjectName (inName);

            // duplicate connections internal (signal->slot; signal->signal)
            SignalsAndSlots* duplicate = new SignalsAndSlots ("duplicateConnections", this);
            connect (duplicate, SIGNAL (Signal1 ()), duplicate, SIGNAL (Signal2 ()));
            connect (duplicate, SIGNAL (Signal1 ()), duplicate, SIGNAL (Signal2 ()));
            connect (duplicate, SIGNAL (Signal2 ()), duplicate, SLOT (Slot2 ()));
            connect (duplicate, SIGNAL (Signal2 ()), duplicate, SLOT (Slot2 ()));

            // duplicate connections external (signal->slot; signal->signal)
            SignalsOnly* duplicateSignals1 = new SignalsOnly ("externalDuplicateConnections_signals1", duplicate);
            SignalsOnly* duplicateSignals2 = new SignalsOnly ("externalDuplicateConnections_signals2", duplicate);
            SlotsOnly* duplicateSlots = new SlotsOnly ("externalDuplicateConnections_slots", duplicate);
            connect (duplicateSignals1, SIGNAL (SignalX ()), duplicateSignals2, SIGNAL (SignalY ()));
            connect (duplicateSignals1, SIGNAL (SignalX ()), duplicateSignals2, SIGNAL (SignalY ()));
            connect (duplicateSignals2, SIGNAL (SignalY ()), duplicateSlots, SLOT (SlotA ()));
            connect (duplicateSignals2, SIGNAL (SignalY ()), duplicateSlots, SLOT (SlotA ()));

            // internal connections (signal->slot; signal->signal)
            SignalsAndSlots* internals = new SignalsAndSlots ("connections", this);
            connect (internals, SIGNAL (Signal1 ()), internals, SLOT (Slot1 ()));
            connect (internals, SIGNAL (Signal2 ()), internals, SLOT (Slot2 ()));
            connect (internals, SIGNAL (Signal1 ()), internals, SIGNAL (Signal2 ()));

            // external connections (signal->slot; signal->signal)
            SignalsOnly* signalsOnly1 = new SignalsOnly ("externalConnections_signals1", internals);
            SignalsOnly* signalsOnly2 = new SignalsOnly ("externalConnections_signals2", internals);
            SlotsOnly* slotsOnly = new SlotsOnly ("externalConnections_slots", internals);
            connect (signalsOnly1, SIGNAL (SignalX ()), slotsOnly, SLOT (SlotA ()));
            connect (signalsOnly1, SIGNAL (SignalY ()), slotsOnly, SLOT (SlotB ()));
            connect (signalsOnly1, SIGNAL (SignalZ (int)), slotsOnly, SLOT (SlotC (int)));
            connect (signalsOnly1, SIGNAL (SignalX ()), signalsOnly2, SIGNAL (SignalX ()));
            connect (signalsOnly1, SIGNAL (SignalY ()), signalsOnly2, SIGNAL (SignalY ()));
            connect (signalsOnly1, SIGNAL (SignalZ (int)), signalsOnly2, SIGNAL (SignalZ (int)));

            // all connection types (signal->slot)
            SignalsAndSlots* connectionTypes = new SignalsAndSlots ("connectionTypes", this);
            SignalsAndSlots* direct = new SignalsAndSlots ("direct", connectionTypes);
            connect (direct, SIGNAL (Signal1 ()), direct, SLOT (Slot1 ()), Qt::DirectConnection);

            SignalsAndSlots* queued = new SignalsAndSlots ("queued", connectionTypes);
            connect (queued, SIGNAL (Signal2 ()), queued, SLOT (Slot2 ()), Qt::QueuedConnection);

            SignalsAndSlots* blockingQueued = new SignalsAndSlots ("blockingQueued", connectionTypes);
            connect (blockingQueued, SIGNAL (Signal1 ()), blockingQueued, SLOT (Slot1 ()), Qt::BlockingQueuedConnection);

            SignalsAndSlots* unique = new SignalsAndSlots ("unique", connectionTypes);
            connect (unique, SIGNAL (Signal2 ()), unique, SLOT (Slot2 ()), Qt::UniqueConnection);
            connect (unique, SIGNAL (Signal2 ()), unique, SLOT (Slot2 ()), Qt::UniqueConnection);

            SignalsAndSlots* autoCompat = new SignalsAndSlots ("autoCompat", connectionTypes);
            connect (autoCompat, SIGNAL (Signal1 ()), autoCompat, SLOT (Slot1 ()), Qt::AutoCompatConnection);

        }

        // ----------------------------------------------------------------------------------------

        SignalsOnly::SignalsOnly(const QString& inName, QObject* inParent) :
            QObject (inParent)
        {
            setObjectName (inName);
        }

        // ----------------------------------------------------------------------------------------

        SlotsOnly::SlotsOnly(const QString& inName, QObject* inParent) :
            QObject (inParent)
        {
            setObjectName (inName);
        }

        void SlotsOnly::SlotA() {
        }

        void SlotsOnly::SlotB() {
        }

        void SlotsOnly::SlotC(int) {
        }

        // ----------------------------------------------------------------------------------------

        SignalsAndSlots::SignalsAndSlots(const QString& inName, QObject* inParent) :
            QObject (inParent)
        {
            setObjectName (inName);
        }

        void SignalsAndSlots::Slot1() {
        }

        void SignalsAndSlots::Slot2() {
        }

    } // namespace test
} // namespace conan
