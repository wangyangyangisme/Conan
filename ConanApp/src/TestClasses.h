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
    \brief Contains declarations for various test classes
*/


#ifndef _TESTCLASSES__31_03_09__01_53_37__H_
#define _TESTCLASSES__31_03_09__01_53_37__H_


#include <QtCore/QObject>


namespace conan {

    //! Contains helper classes for testing Conan
    namespace test {

        class Hierarchy : public QObject
        {
        public:
            Hierarchy(const QString& inName, QObject* inParent = 0);
        };

        // ----------------------------------------------------------------------------------------

        class SignalsOnly : public QObject
        {
            Q_OBJECT

        public:
            SignalsOnly(const QString& inName, QObject* inParent = 0);

        signals:
            void SignalX();
            void SignalY();
            void SignalZ(int);
        };

        // ----------------------------------------------------------------------------------------

        class SlotsOnly : public QObject
        {
            Q_OBJECT

        public:
            SlotsOnly(const QString& inName, QObject* inParent = 0);

        public slots:
            void SlotA();
            void SlotB();
            void SlotC(int);
        };

        // ----------------------------------------------------------------------------------------

        class SignalsAndSlots : public QObject
        {
            Q_OBJECT

        public:
            SignalsAndSlots(const QString& inName, QObject* inParent = 0);

        signals:
            void Signal1();
            void Signal2();

        public slots:
            void Slot1();
            void Slot2();
        };

    } // namespace test

} // namespace conan


#endif //_TESTCLASSES__31_03_09__01_53_37__H_
