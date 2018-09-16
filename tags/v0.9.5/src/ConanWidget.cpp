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
#include "SignalSpy.h"
#include <algorithm>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QStringListModel>
#include <QtGui/QUndoStack>


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
        \brief Creates and initializes the main Conan widget
    */
    ConanWidget::ConanWidget (QWidget* inParent, Qt::WindowFlags inFlags) :
        QWidget (inParent, inFlags),
        mObjectModel (0),
        mSignalModel (0),
        mSlotModel (0),
        mProxySignalModel (0),
        mProxySlotModel (0),
        mInheritanceModel (0),
        mSignalSpyModel (0),
        mUndoStack (0),
        mBlockSelectionCommand (false)
    {
        mForm.setupUi (this);
        InitObjectHierarchyTab ();
        InitSignalSpiesTab ();
    }

    /*!
        \brief Initializes the \a Objects \a hierarchy tab
    */
    void ConanWidget::InitObjectHierarchyTab () {
        // object hierarchy model and view
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
        connect (mForm.signalTree, SIGNAL (customContextMenuRequested (const QPoint&)),
                this, SLOT (SlotConnectionContextMenuRequested (const QPoint&)));
        connect (mForm.slotTree, SIGNAL (customContextMenuRequested (const QPoint&)),
                this, SLOT (SlotConnectionContextMenuRequested (const QPoint&)));
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
        \brief Initializes the \a Signal \a spies tab
    */
    void ConanWidget::InitSignalSpiesTab () {
        // signal spy model and view
        mSignalSpyModel = new SignalSpyModel (this);
        mProxySignalSpyModel = new QSortFilterProxyModel (this);
        mProxySignalSpyModel->setDynamicSortFilter (true);
        mProxySignalSpyModel->setSourceModel (mSignalSpyModel);
        mForm.signalSpiesTableView->setModel (mProxySignalSpyModel);
        mForm.signalSpiesTableView->sortByColumn (SignalSpyModel::kSignature, Qt::AscendingOrder);
        mForm.signalSpiesTableView->verticalHeader ()->hide ();
        mForm.signalSpiesTableView->verticalHeader ()->setResizeMode (QHeaderView::ResizeToContents);
        mForm.signalSpiesTableView->horizontalHeader ()->setResizeMode (QHeaderView::ResizeToContents);
        mForm.signalSpiesTableView->horizontalHeader ()->setHighlightSections (false);
        mForm.signalSpiesTableView->horizontalHeader ()->setMovable (true);

        // connections
        connect (mForm.timestampCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.objectCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.addressCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.signatureCcheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.emitCountCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.argumentsCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.prettyFormattingCheckBox, SIGNAL (toggled (bool)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.separatorLineEdit, SIGNAL (textChanged (const QString&)), this, SLOT (SlotUpdateSignalLoggerOptions ()));
        connect (mForm.signalSpiesTableView, SIGNAL (customContextMenuRequested (const QPoint&)),
                 this, SLOT (SlotSpiesContextMenuRequested (const QPoint&)));
        connect (mForm.actionSelectAllSpies, SIGNAL (triggered ()), mForm.signalSpiesTableView, SLOT (selectAll ()));
        connect (mForm.actionDeleteSpies, SIGNAL (triggered ()), this, SLOT (SlotDeleteSpies ()));
        // logger
        SlotUpdateSignalLoggerOptions ();
        // add actions that have a shortcut
        mForm.signalSpiesTableView->addAction (mForm.actionSelectAllSpies);
        mForm.signalSpiesTableView->addAction (mForm.actionDeleteSpies);
    }


    /*!
        \brief Shows the 'about' dialog
    */
    void ConanWidget::SlotAbout () {
        AboutDialog dialog (this);
        dialog.exec ();
    }

    /*!
        \brief Displays a context menu for a connected method of either the signal or slot view
    */
    void ConanWidget::SlotConnectionContextMenuRequested (const QPoint& inPos) {
        // obtain index and model
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
        QModelIndex methodProxyIndex = view->indexAt (inPos);
        if (methodProxyIndex != view->currentIndex ()) {
            return;
        }
        QModelIndex methodIndex = proxyModel->mapToSource (methodProxyIndex);
        const MethodData* methodData = model->GetMethodData (methodIndex);
        // obtain current object
        QModelIndex objectIndex = mForm.objectTree->currentIndex ();
        QPointer <QObject> object = 0;
        if (methodData) {
            object = methodData->mObject;
        }
        else {
            object = const_cast <QObject*> (mObjectModel->GetObject (objectIndex));
        }
        // obtain current signature
        QModelIndex signatureIndex = methodIndex.sibling (methodIndex.row (), ConnectionModel::kSignature);
        QString signature = model->data (signatureIndex, Qt::EditRole).toString ();
        // populate menu
        QMenu menu;
        menu.addAction (mForm.actionFindMethod);
        menu.addSeparator ();
        menu.addAction (mForm.actionSpySignal);
        mForm.actionFindMethod->setEnabled (methodData);
        mForm.actionSpySignal->setEnabled (false);
        mForm.actionSpySignal->setChecked (false);
        if (model->IsSignal (proxyModel->mapToSource (methodProxyIndex))) {
            if (object) {
                mForm.actionSpySignal->setEnabled (true);
                mForm.actionSpySignal->setChecked (mSignalSpyModel->ContainsSignalSpy (object, signature));
            }
        }
        // show menu
        QPoint pos = view->mapToGlobal (inPos);
        QAction* action = menu.exec (pos);
        // process action
        if (action == mForm.actionFindMethod) {
            SlotFindMethod (methodProxyIndex);
        }
        else if (action == mForm.actionSpySignal) {
            if (object) {
                if (mForm.actionSpySignal->isChecked ()) {
                    mSignalSpyModel->CreateSignalSpy (object, signature);
                }
                else {
                    mSignalSpyModel->DestroySignalSpy (object, signature);
                }
            }
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
        else {
            // Back or Forward toolbar button was pressed
            mForm.objectTree->scrollTo (inCurrent, QAbstractItemView::PositionAtCenter);
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
                arg (ObjectUtility::QualifiedName (methodCopy.mClass, methodCopy.mName)).
                arg (methodCopy.mAddress);
            if (QMessageBox::No ==
                QMessageBox::question (this, "No object found", question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
            {
                return;
            }
            wc.Enable ();
            // locate the top level parent of the object
            QModelIndex index = mForm.objectTree->currentIndex ();
            QPointer <QObject> object = methodCopy.mObject;
            if (object && object->parent ()) {
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
                mForm.signalTree->scrollTo (connectionIndex, QAbstractItemView::PositionAtCenter);
            }
        }
        else if (methodCopy.mMethodType == QMetaMethod::Slot) {
            QModelIndexList matchList = mSlotModel->match (mSlotModel->index (0, 0), Qt::DisplayRole, methodCopy.mSignature, 1, Qt::MatchFixedString);
            if (!matchList.isEmpty ()) {
                QModelIndex connectionIndex = mProxySlotModel->mapFromSource (matchList.first ());
                mForm.slotTree->setCurrentIndex (connectionIndex);
                mForm.slotTree->expand (connectionIndex);
                mForm.slotTree->setFocus (Qt::OtherFocusReason);
                mForm.slotTree->scrollTo (connectionIndex, QAbstractItemView::PositionAtCenter);
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
                    mForm.signalTree->scrollTo (signalIndex, QAbstractItemView::PositionAtCenter);
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
                    mForm.slotTree->scrollTo (slotIndex, QAbstractItemView::PositionAtCenter);
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
                    mForm.signalTree->scrollTo (signalIndex, QAbstractItemView::PositionAtCenter);
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
                    mForm.slotTree->scrollTo (slotIndex, QAbstractItemView::PositionAtCenter);
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

    /*!
        \brief Displays a context menu for the signal spies view
    */
    void ConanWidget::SlotSpiesContextMenuRequested (const QPoint& inPos) {
        QMenu menu;
        menu.addAction (mForm.actionSelectAllSpies);
        menu.addSeparator ();
        menu.addAction (mForm.actionDeleteSpies);
        mForm.actionSelectAllSpies->setEnabled (mSignalSpyModel->rowCount ());
        mForm.actionDeleteSpies->setEnabled (mForm.signalSpiesTableView->selectionModel ()->hasSelection ());
        // show menu
        QPoint pos = mForm.signalSpiesTableView->mapToGlobal (inPos);
        menu.exec (pos);
    }

    /*!
        \brief Synchronizes the logger options with the corresponding settings on \a Signal \a spies tab
    */
    void ConanWidget::SlotUpdateSignalLoggerOptions () {
        mSignalSpyModel->SetLogOptions (
            mForm.timestampCheckBox->isChecked (),
            mForm.objectCheckBox->isChecked (),
            mForm.addressCheckBox->isChecked (),
            mForm.signatureCcheckBox->isChecked (),
            mForm.emitCountCheckBox->isChecked (),
            mForm.argumentsCheckBox->isChecked (),
            mForm.prettyFormattingCheckBox->isChecked (),
            mForm.separatorLineEdit->text ().isEmpty () ? ' ' : mForm.separatorLineEdit->text ().at (0));

        mForm.signalLogLabel->setText (mSignalSpyModel->GetExampleLog ());
    }

    /*
        \brief Destroyes all signal spies that are selected in the signal spies table
    */
    void ConanWidget::SlotDeleteSpies () {
        while (mForm.signalSpiesTableView->selectionModel ()->hasSelection ()) {
            QItemSelection selection = mProxySignalSpyModel->mapSelectionToSource (mForm.signalSpiesTableView->selectionModel ()->selection ());
            QItemSelectionRange range = selection.first ();
            if (range.isValid ()) {
                mSignalSpyModel->removeRows (range.top (), range.height (), range.parent ());
            }
        }
    }

    /*!
        \brief Finds and selects the next object the matches the given text.
    */
    template <typename Pred>
    bool ConanWidget::FindAndSelectObject (Pred inPred) {
        QModelIndex current = mForm.objectTree->currentIndex ();
        QModelIndex result = mObjectModel->FindObject (inPred, current);
        if (result.isValid ()) {
            mForm.objectTree->setCurrentIndex (result);
            mForm.objectTree->setFocus (Qt::OtherFocusReason);
            mForm.objectTree->scrollTo (result, QAbstractItemView::PositionAtCenter);
            return true;
        }
        return false;
    }

} // namespace conan
