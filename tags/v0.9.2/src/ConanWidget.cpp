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
    \brief Contains ConanWidget definition
*/


#include "AboutDialog.h"
#include "ConanWidget.h"
#include "ConanWidget_p.h"
#include "ConnectionModel.h"
#include "ObjectModel.h"
#include "ObjectUtility.h"
#include <algorithm>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QStringListModel>
#include <QUndoStack>


namespace conan {

    //! Contains selection related commands
	namespace /*unnamed*/ {

        /*!
            \brief A command that finds an object by address, selects it and makes it current.

            Note that during the first redo nothing is done, because this command is generated after an actual current index change.
        */
		class SelectObjectCommand : public QUndoCommand {
		public:
			SelectObjectCommand (QItemSelectionModel* inSelectionModel, const QString& inOldAddress, const QString& inNewAddress) : 
				QUndoCommand ("select object"),
                mSelectionModel (inSelectionModel),
                mOldAddress (inOldAddress),
                mNewAddress (inNewAddress),
                mSkipFirstSelect (true)
			{}

			virtual void undo () {
				if (mSelectionModel && !mSkipFirstSelect) {
					if (const ObjectModel* model = dynamic_cast <const ObjectModel*> (mSelectionModel->model ())) {
						QModelIndex objectIndex = model->FindObject (MatchObjectByValue (mNewAddress));
						mSelectionModel->setCurrentIndex (objectIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
					}
				}
				else {
					mSkipFirstSelect = false;
				}
				std::swap (mOldAddress, mNewAddress);
			}
			virtual void redo () {
				undo ();
			}

		private:
			QItemSelectionModel* mSelectionModel;   //! The selection model of an ObjectModel
			QString mOldAddress;                    //! The address of the old current object
			QString mNewAddress;                    //! The address of the new current object
			bool mSkipFirstSelect;                  //! Used to skip the first redo
		};


		// --------------------------------------------------------------------------------------------


        /*!
            \brief A command that toggles a boolean variable.
        */
		class ToggleCommand : public QUndoCommand {
		public:
			ToggleCommand (bool& inVariable) : 
				QUndoCommand ("toggle variable"), mVariable (inVariable)
			{}
	        
			virtual void undo () {
				mVariable = !mVariable;
			}
	        
			virtual void redo () {
				undo ();
			}
		private:
			bool& mVariable;    //! The boolean variable
		};
		
		
		// --------------------------------------------------------------------------------------------
		
		
		/*!
		    \brief Helper class for changing the cursor to a wait cursor.
		*/
		class WaitCursor {
		public:
            WaitCursor () : mEnabled (false) {
		        Enable ();
		    }
		    ~WaitCursor () {
		        Disable ();
		    }
            void Disable () {
                if (mEnabled) {
                    mEnabled = false;
                    QApplication::restoreOverrideCursor ();
                }
            }
            void Enable () {
                if (!mEnabled) {
                    mEnabled = true;
                    QApplication::setOverrideCursor (Qt::WaitCursor);
                }
            }
        private:
            bool mEnabled;
		};


        // ----------------------------------------------------------------------------------------


        /*!
            \brief A unary function object that returns true when an ObjectItem has duplicate connections.
        */
        struct ContainsDuplicateConnections : public std::unary_function <const ObjectItem*, bool>
        {
            bool operator () (const ObjectItem* inItem) {
                if (inItem) {
                    if (QObject* object = const_cast <QObject*> (inItem->Object ())) {
                        // always lock the object before calling any priv functions 
                        priv::ObjectLocker lock (object);
                        // check signals
                        std::vector <ConnectionData> signalData;
                        priv::BuildSignalData (object, signalData);
                        if (std::count_if (
                                signalData.begin (), 
                                signalData.end (),
                                std::mem_fun_ref (&ConnectionData::ContainsDuplicateConnections)))
                        {
                            return true;
                        }
                        // check slots
                        std::vector <ConnectionData> slotData;
                        priv::BuildSlotData (object, slotData);
                        if (std::count_if (
                                slotData.begin (), 
                                slotData.end (),
                                std::mem_fun_ref (&ConnectionData::ContainsDuplicateConnections)))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
        };
    }


	// --------------------------------------------------------------------------------------------


    /*!
        \brief Creates and initialized the main Conan widget
    */
	ConanWidget::ConanWidget (QWidget* inParent, Qt::WindowFlags inFlags) : 
		QWidget (inParent, inFlags),
        mObjectModel (0),
        mSignalModel (0),
        mSlotModel (0),
        mUndoStack (0),
        mBlockSelectionCommand (false)
	{
		mForm.setupUi (this);
        // object model and view
        mObjectModel = new ObjectModel (this);
        mForm.objectTree->setModel (mObjectModel);
        mForm.objectTree->setSortingEnabled (true);
        mForm.objectTree->sortByColumn (ObjectModel::kObject, Qt::AscendingOrder);
        mForm.objectTree->header ()->setResizeMode (QHeaderView::ResizeToContents);
        // signal model and view
        mSignalModel = new ConnectionModel (this);
        mProxySignalModel = new ConnectionFilterProxyModel (this);
		mProxySignalModel->setDynamicSortFilter (true);
		mProxySignalModel->setSourceModel (mSignalModel);
        mForm.signalTree->setModel (mProxySignalModel);
        mForm.signalTree->setSortingEnabled (true);
        mForm.signalTree->sortByColumn (ConnectionModel::kAccess, Qt::AscendingOrder);
        mForm.signalTree->header ()->moveSection (ConnectionModel::kAccess, 0);
		mForm.signalTree->header ()->setResizeMode (QHeaderView::ResizeToContents);
        // slot model and view
        mSlotModel = new ConnectionModel (this);
        mProxySlotModel = new ConnectionFilterProxyModel (this);
		mProxySlotModel->setDynamicSortFilter (true);
		mProxySlotModel->setSourceModel (mSlotModel);
        mForm.slotTree->setModel (mProxySlotModel);
        mForm.slotTree->setSortingEnabled (true);
		mForm.slotTree->sortByColumn (ConnectionModel::kAccess, Qt::AscendingOrder);
        mForm.slotTree->header ()->moveSection (ConnectionModel::kAccess, 0);
        mForm.slotTree->header ()->setResizeMode (QHeaderView::ResizeToContents);
        // inheritance model and view
        mInheritanceModel = new QStringListModel (this);
        mForm.inheritanceList->setModel (mInheritanceModel);
	    // main splitter
		mForm.mainSplitter->setStretchFactor (0, 1);
		mForm.mainSplitter->setStretchFactor (1, 2);
        // undo stack
		mUndoStack = new QUndoStack (this);
        // connect actions to tool buttons
		mForm.backToolButton->setDefaultAction (mForm.actionBack);
		mForm.forwardToolButton->setDefaultAction (mForm.actionForward);
		mForm.findToolButton->setDefaultAction (mForm.actionFind);
		mForm.refreshToolButton->setDefaultAction (mForm.actionRefresh);
		mForm.discoverToolButton->setDefaultAction (mForm.actionDiscover);
        mForm.bugToolButton->setDefaultAction (mForm.actionBug);
		mForm.aboutToolButton->setDefaultAction (mForm.actionAboutConan);
        // connections
        connect (mForm.objectTree->selectionModel (), SIGNAL (currentChanged (const QModelIndex&, const QModelIndex&)),
				 this, SLOT (SlotCurrentObjectChanged (const QModelIndex&, const QModelIndex&)));
        connect (mForm.signalTree, SIGNAL (activated (const QModelIndex&)),
				this, SLOT (SlotFindMethod (const QModelIndex&)));
        connect (mForm.signalTree, SIGNAL (customContextMenuRequested (const QPoint&)),
				this, SLOT (SlotContextMenuRequested (const QPoint&)));
		connect (mForm.slotTree, SIGNAL (activated (const QModelIndex&)),
				this, SLOT (SlotFindMethod (const QModelIndex&)));
		connect (mForm.slotTree, SIGNAL (customContextMenuRequested (const QPoint&)),
				this, SLOT (SlotContextMenuRequested (const QPoint&)));
		connect (mForm.hideInactiveCheckBox, SIGNAL (toggled (bool)),
				 mProxySignalModel, SLOT (SlotEnableInactiveFiltering (bool)));
		connect (mForm.hideInheritedCheckBox, SIGNAL (toggled (bool)),
				 mProxySignalModel, SLOT (SlotEnableInheritedFiltering (bool)));
		connect (mForm.hideInactiveCheckBox, SIGNAL (toggled (bool)),
				 mProxySlotModel, SLOT (SlotEnableInactiveFiltering (bool)));
		connect (mForm.hideInheritedCheckBox, SIGNAL (toggled (bool)),
				 mProxySlotModel, SLOT (SlotEnableInheritedFiltering (bool)));
        connect (mUndoStack, SIGNAL (canUndoChanged (bool)), mForm.actionBack, SLOT (setEnabled (bool)));
		connect (mForm.actionBack, SIGNAL (triggered ()), mUndoStack, SLOT (undo ()));
		connect (mUndoStack, SIGNAL (canRedoChanged (bool)), mForm.actionForward, SLOT (setEnabled (bool)));
		connect (mForm.actionForward, SIGNAL (triggered ()), mUndoStack, SLOT (redo ()));
		connect (mForm.actionFind, SIGNAL (triggered ()), this, SLOT (SlotFindObject ()));
		connect (mForm.actionRefresh, SIGNAL (triggered ()), this, SLOT (SlotRefresh ()));
		connect (mForm.actionFocusFind, SIGNAL (triggered ()), mForm.findLineEdit, SLOT (setFocus ()));
		connect (mForm.actionFocusFind, SIGNAL (triggered ()), mForm.findLineEdit, SLOT (selectAll ()));
		connect (mForm.findLineEdit, SIGNAL (returnPressed ()), this, SLOT (SlotFindObject ()));
		connect (mForm.actionDiscover, SIGNAL (triggered ()), this, SLOT (SlotDiscoverObjects ()));
        connect (mForm.actionBug, SIGNAL (triggered ()), this, SLOT (SlotFindDuplicateConnection ()));
		connect (mForm.actionAboutConan, SIGNAL (triggered ()), this, SLOT (SlotAbout ()));
		// add actions that have a shortcut
		addAction (mForm.actionBack);
		addAction (mForm.actionForward);
		addAction (mForm.actionFind);
		addAction (mForm.actionRefresh);
		addAction (mForm.actionFocusFind);
	}

	/*!
        \brief Shows Conan about dialog
    */
	void ConanWidget::SlotAbout () {
        AboutDialog dialog (this);
        dialog.exec ();
	}

    /*!
        \brief Displays a context menu for a connected method of either the signal or slot view
    */
	void ConanWidget::SlotContextMenuRequested (const QPoint& inPos) {
		QMenu menu;
		menu.addAction (mForm.actionFindMethod);
		QPoint pos;
		const QAbstractItemView* view = dynamic_cast <QAbstractItemView*> (sender ());
		if (!view) {
			return;
		}
		const ConnectionFilterProxyModel* proxyModel = dynamic_cast <ConnectionFilterProxyModel*> (view->model ());
		if (!proxyModel) {
			return;
		}
		const ConnectionModel* model = dynamic_cast <ConnectionModel*> (proxyModel->sourceModel ());
		if (!model) {
			return;
		}
		QModelIndex index = view->indexAt (inPos);
		if (index != view->currentIndex ()) {
			return;
		}
		if (!model->GetMethodData (proxyModel->mapToSource (index))) {
			return;
		}
		pos = view->mapToGlobal (inPos);
		if (menu.exec (pos) == mForm.actionFindMethod) {
			SlotFindMethod (index);
		}
	}

    /*!
        \brief Called when the current object has changed.

        Initializes the signal, slot and inheritance view with the current object
        and creates a command to undo the current object change.
    */
	void ConanWidget::SlotCurrentObjectChanged (const QModelIndex& inCurrent, const QModelIndex& inPrevious) {
		QStringList inheritanceData;
		std::vector <ConnectionData> signalData;
		std::vector <ConnectionData> slotData;

		QObject* object = const_cast <QObject*> (mObjectModel->GetObject (inCurrent));
		if (object) {
    		//object->dumpObjectInfo ();  // for debugging conan

            // always lock the object before calling any priv functions 
			priv::ObjectLocker lock (object);
			priv::BuildSignalData (object, signalData);
			priv::BuildSlotData (object, slotData);
			priv::BuildInheritanceData (object, inheritanceData);
		}
        // init all models
		mSignalModel->SetData (signalData);
		mSlotModel->SetData (slotData);
		mInheritanceModel->setStringList (inheritanceData);

		QString className = ObjectUtility::Class (object);
		mProxySignalModel->SetClassName (className);
		mProxySlotModel->SetClassName (className);
		if (!mBlockSelectionCommand) {
            // create a command to undo the current object change
            // but only when this change was not triggered by a command
			QString previousAddress = mObjectModel->GetAddress (inPrevious);
			QString currentAddress = mObjectModel->GetAddress (inCurrent);
			mUndoStack->beginMacro ("select");
			mUndoStack->push (new ToggleCommand (mBlockSelectionCommand));
			mUndoStack->push (new SelectObjectCommand (mForm.objectTree->selectionModel (), previousAddress, currentAddress));
			mUndoStack->push (new ToggleCommand (mBlockSelectionCommand));
			mUndoStack->endMacro ();
		}
	}

    /*!
        \brief Adds the object hierarchy defined by the given object to the object tree.
        Note that the object hierarchy may be merged with an existing hierarchy, see RootItem::AddChild for details.
    */
	void ConanWidget::AddRootObject (const QObject* inObject) {
		if (mObjectModel) {
			mObjectModel->AddRootObject (inObject);
		}
	}

    /*!
        \brief Discovers exting object hierarchies and adds these to the object tree.
        Note that currently only hierarchies starting with a QWidget are found.
    */
    void ConanWidget::DiscoverObjects () {
		QString address = mObjectModel->GetAddress (mForm.objectTree->currentIndex ());
	    
		mObjectModel->DiscoverRootObjects ();
		mSignalModel->SetData (std::vector <ConnectionData> ());
		mSlotModel->SetData (std::vector <ConnectionData> ());

		bool oldBlock = BlockSelectionCommand (true);
		FindAndSelectObject (MatchObjectByValue (address));
		BlockSelectionCommand (oldBlock);
	}

    /*!
        \brief The complete object hierarchy is refreshed.
        Resets all views, but keeps the current object selected.
    */
	void ConanWidget::SlotRefresh () {
	    WaitCursor wc;

		QString address = mObjectModel->GetAddress (mForm.objectTree->currentIndex ());
	    
		mObjectModel->SlotRefresh ();
		mSignalModel->SetData (std::vector <ConnectionData> ());
		mSlotModel->SetData (std::vector <ConnectionData> ());

		bool oldBlock = BlockSelectionCommand (true);
		FindAndSelectObject (MatchObjectByValue (address));
		BlockSelectionCommand (oldBlock);
	}

    /*!
        \brief Finds and selects the next object the matches the search text.
    */
	void ConanWidget::SlotFindObject () {
	    WaitCursor wc;	    
		QString text = mForm.findLineEdit->text ();
		if (!FindAndSelectObject (MatchObjectByValue (text))) {
            wc.Disable ();
			QMessageBox::information (this, "No object found", "The following text could not be matched with an object:\n\n" + text);
		}
	}


    /*!
        \brief Sets the selcection command block to \p inBlock and returns its previous value.
    */
    bool ConanWidget::BlockSelectionCommand (bool inBlock) {
		bool oldBlock = mBlockSelectionCommand;
		mBlockSelectionCommand = inBlock;
		return oldBlock;
	}

    /*!
        \brief Finds and selects the object corresponding to the selected method in either the signal or slot view.
        If the object cannot be found the user may choose to add the corresponding object hierarachy to the object tree.
    */
	void ConanWidget::SlotFindMethod (const QModelIndex& inIndex) {
	    WaitCursor wc;
        // retrieve selected method
		const MethodData* methodData = 0;
		if (mForm.signalTree == sender ()) {
			methodData = mSignalModel->GetMethodData (mProxySignalModel->mapToSource (inIndex));
		}
		else if (mForm.slotTree == sender ()) {
			methodData = mSlotModel->GetMethodData (mProxySlotModel->mapToSource (inIndex));
		}
		if (!methodData) {
			return;
		}
        // find and select the corresponding object
		MethodData methodCopy = *methodData;
		if (!FindAndSelectObject (MatchObjectByValue (methodCopy.mAddress))) {
            wc.Disable ();
			QString question = QString (
				"The following object could not be found: \n"
				"%1 <%2> \n\n"
				"Do you wish to add the corresponding object hierarchy?").
				arg (methodCopy.mObject).
				arg (methodCopy.mAddress);
			if (QMessageBox::No ==
				QMessageBox::question (this, "No object found", question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
			{
				return;
			}
            wc.Enable ();
            // locate the top level parent of the object	    
			QModelIndex index = mForm.objectTree->currentIndex ();
			const QObject* object = mObjectModel->GetObject (index);
			if (mForm.signalTree == sender ()) {
				object = priv::FindReceiver (object, methodCopy.mAddress);
			}
			else if (mForm.slotTree == sender ()) {
				object = priv::FindSender (object, methodCopy.mAddress);
			}
			if (object->parent ()) {
				// locate top level parent
				while (object->parent ()) {
					object = object->parent ();
				}
			}
            // add the hierarchy to the object tree
			mObjectModel->AddRootObject (object);
            // retry selecting the object
			if (!FindAndSelectObject (MatchObjectByValue (methodCopy.mAddress))) {
				return;
			}
		}
        // find the method in either the signal or slot view and expand it
		if (methodCopy.mMethodType == QMetaMethod::Signal) {
			QModelIndexList matchList = mSignalModel->match (mSignalModel->index (0, 0), Qt::DisplayRole, methodCopy.mSignature, 1, Qt::MatchFixedString);
			if (!matchList.isEmpty ()) {
				QModelIndex connectionIndex = mProxySignalModel->mapFromSource (matchList.first ());
				mForm.signalTree->setCurrentIndex (connectionIndex);
				mForm.signalTree->expand (connectionIndex);
				mForm.signalTree->setFocus (Qt::OtherFocusReason);
			}
		}
		else if (methodCopy.mMethodType == QMetaMethod::Slot) {
			QModelIndexList matchList = mSlotModel->match (mSlotModel->index (0, 0), Qt::DisplayRole, methodCopy.mSignature, 1, Qt::MatchFixedString);
			if (!matchList.isEmpty ()) {
				QModelIndex connectionIndex = mProxySlotModel->mapFromSource (matchList.first ());
				mForm.slotTree->setCurrentIndex (connectionIndex);
				mForm.slotTree->expand (connectionIndex);
				mForm.slotTree->setFocus (Qt::OtherFocusReason);
			}
		}
	}

    /*!
        \brief Finds and selects the next object the contains duplicate connections.
    */
    void ConanWidget::SlotFindDuplicateConnection () {
        WaitCursor wc;
        bool done = false;
        // check the current object's signals 
        QModelIndex currentSignal = mForm.signalTree->currentIndex ();
        if (currentSignal.isValid ()) {
            if (currentSignal.parent ().isValid ()) {
                currentSignal = currentSignal.parent ();
            }
            // select and expand the next signal that has duplicate connections
            int rowCount = mProxySignalModel->rowCount ();
            for (int r=currentSignal.row ()+1; r<rowCount; r++) {
                QModelIndex signalIndex = currentSignal.sibling (r, 0);
                if (mSignalModel->ContainsDuplicateConnections (mProxySignalModel->mapToSource (signalIndex))) {
                    mForm.signalTree->setCurrentIndex (signalIndex);
                    mForm.signalTree->expand (signalIndex);
                    mForm.signalTree->setFocus (Qt::OtherFocusReason);
                    done = true;
                    break;
                }
            }
        }
        // check the current object's slots
        QModelIndex currentSlot = mForm.slotTree->currentIndex ();
        if (currentSlot.isValid ()) {
            if (currentSlot.parent ().isValid ()) {
                currentSlot = currentSlot.parent ();
            }
            // select and expand the next slot that has duplicate connections
            int rowCount = mProxySlotModel->rowCount ();
            for (int r=currentSlot.row ()+1; r<rowCount; r++) {
                QModelIndex slotIndex = currentSlot.sibling (r, 0);
                if (mSlotModel->ContainsDuplicateConnections (mProxySlotModel->mapToSource (slotIndex))) {
                    mForm.slotTree->setCurrentIndex (slotIndex);
                    mForm.slotTree->expand (slotIndex);
                    mForm.slotTree->setFocus (Qt::OtherFocusReason);
                    done = true;
                    break;
                }
            }
        }
        if (done) {
            return;
        }
        // find another object with duplicate connections
		if (!FindAndSelectObject (ContainsDuplicateConnections ())) {
            wc.Disable ();
			QMessageBox::information (this, "No object found", "No object with duplicate connections could be found");
		}
        else {
            // select and expand the first signal that has duplicate connections
            int signalCount = mProxySignalModel->rowCount ();
            for (int r=0; r<signalCount; r++) {
                QModelIndex signalIndex = mProxySignalModel->index (r, 0);
                if (mSignalModel->ContainsDuplicateConnections (mProxySignalModel->mapToSource (signalIndex))) {
                    mForm.signalTree->setCurrentIndex (signalIndex);
                    mForm.signalTree->expand (signalIndex);
                    mForm.signalTree->setFocus (Qt::OtherFocusReason);
                    return;
                }
            }
            // select and expand the first slot that has duplicate connections
            int slotCount = mProxySlotModel->rowCount ();
            for (int r=0; r<slotCount; r++) {
                QModelIndex slotIndex = mProxySlotModel->index (r, 0);
                if (mSlotModel->ContainsDuplicateConnections (mProxySlotModel->mapToSource (slotIndex))) {
                    mForm.slotTree->setCurrentIndex (slotIndex);
                    mForm.slotTree->expand (slotIndex);
                    mForm.slotTree->setFocus (Qt::OtherFocusReason);
                    return;
                }
            }
        }
    }

    /*!
        \brief Discovers exting object hierarchies and adds these to the object tree.
    */
	void ConanWidget::SlotDiscoverObjects () {
	    WaitCursor wc;
        DiscoverObjects ();
	}
} // namespace conan
