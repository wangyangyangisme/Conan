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
    \brief Contains ConanWidget declaration
*/


#ifndef _ConanWidget__09_12_08__12_00_55__H_
#define _ConanWidget__09_12_08__12_00_55__H_


#include "ConanDefines.h"
#include "ui_ConanWidget.h"
#include <QWidget>


class QUndoStack;
class QStringListModel;


namespace conan {
	class ObjectModel;
	class ConnectionModel;
	class ConnectionFilterProxyModel;


    /*!
        \brief The main Conan widget that provides run-time signal/slot introspection.

        Object hierarchise can be added using \p AddRootObject or they can be discover automatically using \p DiscoverObjects.

        The following shortcuts are supported:
            - Alt+Left - Go back to previous selected object
            - Alt+Right - Go forward to next selected object
            - Ctrl+F - Focus the find control to enter a search text
            - F3 - Find next matching object
            - F5 - Refresh all object hierarchies
            - Enter
                - When typing a search text - performs the actual search.
                - When selecting a method connected to signal or slot - Finds the corresponding object (same applies to double-click).
    */
	class CONAN_API ConanWidget : public QWidget {
		Q_OBJECT

	public:
		ConanWidget (QWidget* inParent = 0, Qt::WindowFlags inFlags = 0);   

		void AddRootObject (const QObject* inObject);
        void DiscoverObjects ();

	private:
		bool FindAndSelectObject (const QString& inValue);
		bool BlockSelectionCommand (bool inBlock);

	private slots:
		void SlotAbout ();
		void SlotCurrentObjectChanged (const QModelIndex& inCurrent, const QModelIndex& inPrevious);
		void SlotRefresh ();
		void SlotFindObject ();
		void SlotFindMethod (const QModelIndex& inIndex);
		void SlotDiscoverObjects ();
		void SlotContextMenuRequested (const QPoint& inPos);

	public:
		Ui::ConanWidget mForm;
		ObjectModel* mObjectModel;                      //!< The model containing the object hierarchies
		ConnectionModel* mSignalModel;                  //!< The model containing all signal connections for the current object
		ConnectionModel* mSlotModel;                    //!< The model containing all slot connections for the current object
		ConnectionFilterProxyModel* mProxySignalModel;  //!< Provides sorting and filtering for the signal model
		ConnectionFilterProxyModel* mProxySlotModel;    //!< Provides sorting and filtering for the slot model
		QStringListModel* mInheritanceModel;            //!< The model containing the inheritance data for the current object
		QUndoStack* mUndoStack;                         //!< Object selection undo stack
		bool mBlockSelectionCommand;                    //!< Used to prevent a selection command to generate a new selection command
	};
} // namespace conan

#endif //_ConanWidget__09_12_08__12_00_55__H_
