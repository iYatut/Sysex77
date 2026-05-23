/*
  ==============================================================================

    BankTableModel.h
    Created: 17 Nov 2018 6:02:48pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "YamahaLmVoiceDump.h"
#include "Sy99LibrarySession.h"
#include "VoicesTableModel.h"

//==============================================================================
/*
*/

class BankTableModel    : public Component, public ChangeListener,public ChangeBroadcaster, public FileDragAndDropTarget, public TextEditor::Listener
{
public:
    BankTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("BANK");
        sy99PruneLegacyLibrarySyxFiles();
        loadBank();
   
        sourceListBox.setModel (&sourceModel);
       // sourceListBox.getVerticalScrollBar().setColour(ListBox::outlineColourId, SYCol);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        
        addAndMakeVisible (sourceListBox);
        addAndMakeVisible(groupDrop);
        addAndMakeVisible(editText);
        editText.setVisible(false);
        editText.setInputRestrictions(10);
        editText.addListener(this);
        editText.setSelectAllWhenFocused(true);
        
        groupDrop.setVisible(false);
        sourceModel.owner = this;
        sourceModel.addChangeListener(this);
        libraryLoadPersistedSelectionFromDisk();
        restoreSy99LibrarySessionOnStartup();
    }

    ~BankTableModel()
    {
        sourceModel.removeChangeListener(this);
        editText.removeListener(this);
    }
    void textEditorReturnKeyPressed	(	TextEditor & 		) override
    {
                 doubleClickBank = false;
        editText.setVisible(false);
        String str;
        str = editText.getText();
        if(!str.endsWithIgnoreCase (".syx"))
        str = str + ".SYX";
        arrayBank.set(rowSelectedBank,str);
 
        Logger::writeToLog( BankFiles[rowSelectedBank].getFullPathName());
        
    
        BankFiles[rowSelectedBank].moveFileTo (BankFiles[rowSelectedBank].getSiblingFile (str));
        
        loadBank();
        repaint();
    
    }
    
    void textEditorFocusLost	(	TextEditor & 		) override
    {
            editText.setVisible(false);
          doubleClickBank = false;
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showLibraryClearMenu (this);
            return;
        }

        Logger::writeToLog ("Bank Mouse event");
    }

    /** Delete .syx files; returns count removed. */
    int removeLibrarySyxFiles (bool capturesOnly) noexcept
    {
        juce::Array<juce::File> files;

        if (capturesOnly)
        {
            const juce::File cap = libraryCapturesDirPath();

            if (cap.isDirectory())
                cap.findChildFiles (files, juce::File::findFiles, false, "*.syx");
        }
        else if (appDirPath.isDirectory())
        {
            appDirPath.findChildFiles (files, juce::File::findFiles, true, "*.syx");
        }

        int removed = 0;

        for (const auto& f : files)
            if (f.deleteFile())
                ++removed;

        return removed;
    }

    void resetLibraryAfterFilePurge() noexcept
    {
        clearAllLibraryDisplayState();
        loadBank();
        startWithEmptyLibrarySelection();
        sendChangeMessage();
    }

    void showLibraryClearMenu (juce::Component* targetComponent)
    {
        PopupMenu menu;
        menu.addItem (1, String::fromUTF8 (u8"Очистить captures/ (sync, dump…)"));
        menu.addSeparator();
        menu.addItem (2, String::fromUTF8 (u8"Удалить ВСЕ .syx в библиотеке"));

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (targetComponent),
                            [this] (int result)
        {
            if (result == 1)
                confirmAndClearLibrary (true);
            else if (result == 2)
                confirmAndClearLibrary (false);
        });
    }

    void confirmAndClearLibrary (bool capturesOnly)
    {
        const int pending = countLibrarySyxFiles (capturesOnly);

        if (pending == 0)
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                              String::fromUTF8 (u8"Библиотека"),
                                              String::fromUTF8 (capturesOnly
                                                  ? u8"Папка captures/ уже пуста."
                                                  : u8"Файлов .syx в библиотеке нет."));
            return;
        }

        const String detail = String::fromUTF8 (capturesOnly
            ? u8"Удалить "
            : u8"Удалить все ")
            + String (pending)
            + String::fromUTF8 (capturesOnly
                ? u8" файлов .syx из captures/?"
                : u8" файлов .syx из библиотеки (включая captures/)?");

        AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
                                      String::fromUTF8 (u8"Очистка библиотеки"),
                                      detail,
                                      String::fromUTF8 (u8"Удалить"),
                                      String::fromUTF8 (u8"Отмена"),
                                      nullptr,
                                      ModalCallbackFunction::create ([this, capturesOnly] (int r)
        {
            if (r != 1)
                return;

            const int removed = removeLibrarySyxFiles (capturesOnly);
            resetLibraryAfterFilePurge();
            Logger::writeToLog ("[Library] Cleared "
                                + String (removed) + " .syx file(s)"
                                + (capturesOnly ? " from captures/" : ""));

            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                              String::fromUTF8 (u8"Библиотека"),
                                              String::fromUTF8 (u8"Удалено файлов: ")
                                                  + String (removed));
        }));
    }

    int countLibrarySyxFiles (bool capturesOnly) const noexcept
    {
        juce::Array<juce::File> files;

        if (capturesOnly)
        {
            const juce::File cap = libraryCapturesDirPath();

            if (cap.isDirectory())
                cap.findChildFiles (files, juce::File::findFiles, false, "*.syx");
        }
        else if (appDirPath.isDirectory())
        {
            appDirPath.findChildFiles (files, juce::File::findFiles, true, "*.syx");
        }

        return files.size();
    }


    void changeListenerCallback (ChangeBroadcaster* source) override
    {
      if (loadBankRequest)
      {
          loadBankRequest = false;
          reloadAfterLibraryFileAdded();
          return;
      }
        Logger::writeToLog("BankModel: changebrodcaster");
        if (requestSysex == true)
        {
            loadBank();
            repaint();
            requestSysex = false;
            newMessage = false;
        }
        else if(doubleClickBank)
        {
          
            Logger::writeToLog("Change double click");
            editText.setVisible(true);
            editText.setBounds(sourceListBox.getRowPosition(rowSelectedBank, false));
            editText.setText(arrayBank[rowSelectedBank]);
 //           editText.insertTextAtCaret(arrayBank[rowSelectedBank]);
            
        }
        else if(bankDeleteKey) //message delete
        {
            bankDeleteKey = false;
            int rowToDelete = rowSelectedBank;
            int fileRow = sourceListBox.getSelectedRow();
            AlertWindow::showOkCancelBox(
                AlertWindow::WarningIcon, "Erase Bank File",
                "Do you really want erase the bank file ? " + arrayBank[rowToDelete],
                "Yes", "No", nullptr,
                ModalCallbackFunction::create([this, rowToDelete, fileRow](int result) {
                    if (result == 1) {
                        arrayBank.remove(rowToDelete);
                        if(fileRow >= 0 && fileRow < BankFiles.size() && BankFiles[fileRow].exists())
                        {
                            BankFiles[fileRow].deleteFile();
                            BankFiles.remove(fileRow);
                        }
                        repaint();
                    }
                }));
        }
        else
        {
        reloadVoicesFromSelectedBankFile();
        }
    }

    /** Load internal voice grid from SY-99 session capture (AUTOSYNC-VC-INT). */
    void reloadSy99InternalVoiceGrid() noexcept
    {
        resetLibraryRecallContext();

        if (auto* index = sy99EnsureVoiceCaptureIndex (Sy99LibraryContentPage::internalVoices))
        {
            sy99CopyVoiceIndexToInternalArrays (*index);
            bankSelected = kSy99LibrarySessionBankLabel;
            Logger::writeToLog ("[SY-99] Internal grid: "
                              + String (arrayListVoices.size()) + " voice(s)");
        }
        else
        {
            arrayListVoices.clear();
            voiceSysexFileOffsets.clear();
            voiceSysexFileLengths.clear();
        }
    }

    /** Select virtual SY-99 row and refresh all library pages from captures. */
    bool activateSy99LibrarySession() noexcept
    {
        sy99RefreshSy99LibrarySessionFromDisk();
        loadBank();

        if (! sy99LibrarySession().active)
            return false;

        for (int r = 0; r < arrayBank.size(); ++r)
        {
            if (! sy99IsSy99LibrarySessionBankLabel (arrayBank[r]))
                continue;

            sourceListBox.selectRow (r);
            rowSelectedBank = r;
            bankSelected = kSy99LibrarySessionBankLabel;
            reloadSy99InternalVoiceGrid();
            sy99RefreshSy99LibraryNamesFromSession();
            sendChangeMessage();
            return true;
        }

        return false;
    }

    void restoreSy99LibrarySessionOnStartup() noexcept
    {
        sy99RefreshSy99LibrarySessionFromDisk();
        loadBank();

        if (sy99LibrarySession().active)
        {
            for (int r = 0; r < arrayBank.size(); ++r)
            {
                if (! sy99IsSy99LibrarySessionBankLabel (arrayBank[r]))
                    continue;

                sourceListBox.selectRow (r);
                rowSelectedBank = r;
                bankSelected = kSy99LibrarySessionBankLabel;
                reloadSy99InternalVoiceGrid();
                sy99RefreshSy99LibraryNamesFromSession();
                break;
            }
        }
        else
        {
            startWithEmptyLibrarySelection();
        }

        sourceListBox.updateContent();
        repaint();
    }

    /** Empty UI when no SY-99 session and no bank selected. */
    void startWithEmptyLibrarySelection() noexcept
    {
        bankSelected.clear();
        rowSelectedBank = -1;
        bankSelectedVoiceIndex = -1;
        bankSelectedVoiceName.clear();
        arrayListVoices.clear();
        voiceSysexFileOffsets.clear();
        voiceSysexFileLengths.clear();
        sourceListBox.deselectAllRows();
        sourceListBox.updateContent();
        repaint();
    }

    /** Copy .syx into captures/ and make it the active bank. */
    bool importSyxFileToCapturesAndActivate (const juce::File& sourceFile) noexcept
    {
        if (! sourceFile.existsAsFile())
            return false;

        const juce::File cap = sy99EnsureLibraryCapturesDir();
        juce::File dest = cap.getChildFile (sourceFile.getFileName());

        if (dest.existsAsFile())
            dest = dest.getNonexistentSibling();

        if (! sourceFile.copyFileTo (dest))
            return false;

        selectBankFileAndReloadVoices (dest);
        return true;
    }

    /** Rescan library folder; preserve current bank selection (no auto-select of new captures). */
    void reloadAfterLibraryFileAdded()
    {
        const String previousBank = bankSelected;
        loadBank();

        int rowToSelect = -1;

        for (int r = 0; r < arrayBank.size(); ++r)
        {
            if (arrayBank[r].equalsIgnoreCase (previousBank))
            {
                rowToSelect = r;
                break;
            }
        }

        if (rowToSelect >= 0)
        {
            sourceListBox.selectRow (rowToSelect);
            bankSelected = arrayBank[rowToSelect];
        }
        else
        {
            sourceListBox.updateContent();
            repaint();
        }
    }

    /** Select a .syx in BANK (including captures/) and load voices into A–D tables. */
    void selectBankFileAndReloadVoices (const File& file)
    {
        loadBank();

        int rowToSelect = -1;

        for (int r = 0; r < BankFiles.size(); ++r)
        {
            if (BankFiles[r] == file
                || BankFiles[r].getFullPathName().equalsIgnoreCase (file.getFullPathName()))
            {
                rowToSelect = r;
                break;
            }
        }

        if (rowToSelect < 0)
        {
            Logger::writeToLog ("[BankLoad] file not indexed: " + file.getFullPathName());
            sourceListBox.updateContent();
            repaint();
            return;
        }

        sourceListBox.selectRow (rowToSelect);
        rowSelectedBank = rowToSelect;
        reloadVoicesFromSelectedBankFile();
    }

    /** Parse F0…F7 frames from the selected bank .syx into arrayListVoices / offset tables. */
    void reloadVoicesFromSelectedBankFile()
    {
        resetLibraryRecallContext();

        arrayListVoices.clear();
        voiceSysexFileOffsets.clear();
        voiceSysexFileLengths.clear();

        const int row = sourceListBox.getSelectedRow();

        if (row < 0 || row >= BankFiles.size())
        {
            Logger::writeToLog ("[BankLoad] no bank row selected");
            sendChangeMessage();
            return;
        }

        bankSelected = arrayBank[row];
        const File& bankFile = BankFiles[row];

        if (! bankFile.existsAsFile())
        {
            Logger::writeToLog ("[BankLoad] bank file missing: " + bankFile.getFullPathName());
            sendChangeMessage();
            return;
        }

        MemoryBlock mb;

        if (! bankFile.loadFileAsData (mb))
        {
            Logger::writeToLog ("[BankLoad] failed to read: " + bankFile.getFullPathName());
            sendChangeMessage();
            return;
        }

        const uint8* const data = (const uint8*) mb.getData();
        const size_t total = mb.getSize();

        if (total == 0)
        {
            Logger::writeToLog ("[BankLoad] empty file: " + bankFile.getFileName());
            sendChangeMessage();
            return;
        }

        size_t i = 0;

        while (i < total)
        {
            if (data[i] != 0xF0)
            {
                ++i;
                continue;
            }

            const size_t frameStart = i;
            size_t j = i + 1;

            while (j < total && data[j] != 0xF7)
                ++j;

            if (j >= total)
                break;

            const int frameLen = (int) (j - frameStart + 1);

            if (! YamahaLmVoiceDump::frameContainsLmTag (data + frameStart, frameLen, "8101VC"))
            {
                i = j + 1;
                continue;
            }

            String str;

            YamahaLmVoiceDump::Lm8101VcMinimal parsed;

            if (YamahaLmVoiceDump::parseLm8101VcMinimal (data + frameStart, frameLen, parsed)
                && parsed.name[0] != '\0')
            {
                str = String (parsed.name).trimEnd();
            }
            else if (frameStart + 43 <= total)
            {
                for (int a = 0; a < 10; ++a)
                {
                    const uint8 c = data[frameStart + 33 + a];
                    const char ch = (c >= 0x20 && c < 0x7F) ? (char) c : ' ';
                    str += String::charToString ((juce_wchar) ch);
                }

                str = str.trimEnd();
            }

            arrayListVoices.add (str);
            voiceSysexFileOffsets.add ((int) frameStart);
            voiceSysexFileLengths.add (frameLen);

            i = j + 1;
        }

        Logger::writeToLog ("[BankLoad] " + bankFile.getFileName() + ": "
                            + String (arrayListVoices.size()) + " voice(s) (8101VC), "
                            + String ((int64) total) + " bytes");

        if (! arrayListVoices.isEmpty())
        {
            int idx = bankSelectedVoiceIndex;

            if (idx < 0 || idx >= arrayListVoices.size())
                idx = 0;

            selectLibraryVoiceSlot (idx % 16, (idx / 16) * 16);
        }

        sendChangeMessage();
    }

    void selectFirstBankAndReload()
    {
        if (arrayBank.isEmpty())
            return;

        sourceListBox.selectRow (0);
        rowSelectedBank = 0;
        reloadVoicesFromSelectedBankFile();
    }

    void paint (Graphics& g) override
    {
        
       }
    
    void resized() override
    {
    
            sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
        groupDrop.setBoundsInset(BorderSize<int>(10));
        editText.setBounds(0, 10, getWidth(), 24);
        
    };
    
//private:
/*
    struct  ItemTextEditor : public TextEditor
    {

    };
*/
 //==============================================================================
struct SourceItemListboxContents  : public ListBoxModel, public ChangeBroadcaster
    {
        BankTableModel* owner = nullptr;

        // The following methods implement the necessary virtual functions from ListBoxModel,
        // telling the listbox how many rows there are, painting them, etc.
  
        
        int getNumRows() override
        {
            return arrayBank.size();
        }
        void change()
        {
            
        }
        void deleteKeyPressed (int lastRowSelected	) override
        {
            Logger::writeToLog("Bank Delete key pressed");
            bankDeleteKey = true;
            rowSelectedBank = lastRowSelected;
            sendChangeMessage();
        }

        void listBoxItemClicked (int row, const MouseEvent&) override
        {
            rowSelectedBank = row;

            if (row >= 0 && row < arrayBank.size())
                bankSelected = arrayBank[row];

            if (owner != nullptr)
            {
                if (sy99IsSy99LibrarySessionBankLabel (bankSelected))
                {
                    owner->reloadSy99InternalVoiceGrid();
                    sy99RefreshSy99LibraryNamesFromSession();
                    owner->sendChangeMessage();
                    return;
                }

                owner->reloadVoicesFromSelectedBankFile();
            }
        }

        void listBoxItemDoubleClicked	(int 	row,const MouseEvent &)	override
        {
            doubleClickBank = true;
            rowSelectedBank = row;
            sendChangeMessage();
            Logger::writeToLog("table A double clic");
        }
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
           
            if (rowIsSelected)
            {
                g.fillAll (SYColSelected);
            }
                else if (rowNumber % 2)
                {
                    
                g.fillAll (SYColAlt);
                }
                g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                g.setFont (height * 0.7f);
                  
              //    auto text = arrayBank[rowNumber] ;
                    if (rowNumber < arrayBank.size() && arrayBank[rowNumber].isNotEmpty())
                    g.drawFittedText(arrayBank[rowNumber], 0, 0, width, height, Justification::centred, 1);
/*
                    g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                            5, 0, width, height,
                            Justification::centredLeft, true);
 */
                
                }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.setFont(16.0f);
            g.drawFittedText (TRANS("Librairie"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent& e) override
        {
            if (e.mods.isPopupMenu())
            {
                owner.showLibraryClearMenu (this);
                return;
            }

            Logger::writeToLog ("BankTableModel -> clicked");
        }

        
        BankTableModel& owner;
    };
    
    //==============================================================================
    // These methods implement the FileDragAndDropTarget interface, and allow our component
    // to accept drag-and-drop of files..
    
    bool isInterestedInFileDrag (const StringArray& /*files*/) override
    {
        Logger::writeToLog("isInterested");
        // normally you'd check these files to see if they're something that you're
        // interested in before returning true, but for the demo, we'll say yes to anything..
        return true;
    }
    
    void fileDragEnter (const StringArray& /*files*/, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog("fileDragEnter");
        setMouseCursor(MouseCursor::DraggingHandCursor);
        groupDrop.setVisible(true);
        
        repaint();
    }
    
    void fileDragMove (const StringArray& /*files*/, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog("fileDragMove");
    }
    
    void fileDragExit (const StringArray& /*files*/) override
    {
        Logger::writeToLog("fileDragExit");
        groupDrop.setVisible(false);
        repaint();
    }
    
    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog ("fileDropped");
        groupDrop.setVisible (false);

        juce::File lastImported;

        for (int i = 0; i < files.size(); ++i)
        {
            const juce::File audioFile (files[i]);

            if (audioFile.existsAsFile() && audioFile.hasFileExtension ("syx"))
            {
                const juce::File cap = sy99EnsureLibraryCapturesDir();
                juce::File dest = cap.getChildFile (audioFile.getFileName());

                if (dest.existsAsFile() && ! dest.hasIdenticalContentTo (audioFile))
                    dest = dest.getNonexistentSibling();

                if (audioFile.copyFileTo (dest))
                    lastImported = dest;
            }
        }

        if (lastImported.existsAsFile())
            selectBankFileAndReloadVoices (lastImported);

        repaint();
    }

//==============================================================================
    
    void loadBank()
    {
        arrayBank.clear();
        BankFiles.clear();

        if (! appDirPath.exists())
        {
            appDirPath.createDirectory();
            Logger::writeToLog ("Creation du dossier de presets");
            Logger::writeToLog (appDirPath.getFullPathName());
        }

        sy99RefreshSy99LibrarySessionFromDisk();

        juce::Array<juce::File> userImports;
        const juce::File cap = sy99EnsureLibraryCapturesDir();

        if (cap.isDirectory())
        {
            juce::Array<juce::File> capFiles;
            cap.findChildFiles (capFiles, File::findFiles, false, "*.syx");

            for (const auto& f : capFiles)
            {
                if (! sy99IsCanonicalAutoloadCaptureFileName (f.getFileName()))
                    userImports.add (f);
            }
        }

        struct BankFileNewestFirstComparator
        {
            static int compareElements (const File& a, const File& b)
            {
                const int64 ta = a.getLastModificationTime().toMilliseconds();
                const int64 tb = b.getLastModificationTime().toMilliseconds();

                if (ta > tb) return -1;
                if (ta < tb) return 1;
                return 0;
            }
        };

        BankFileNewestFirstComparator bankFileSort;
        userImports.sort (bankFileSort);

        if (sy99LibrarySession().active)
        {
            arrayBank.add (kSy99LibrarySessionBankLabel);
            BankFiles.add (sy99LibrarySession().voiceInternal);
        }

        for (const auto& f : userImports)
        {
            BankFiles.add (f);
            arrayBank.add (f.getRelativePathFrom (appDirPath).replaceCharacter ('\\', '/'));
        }

        numRows = arrayBank.size();
        
        //Verifier si il s'agit bien d'une BANK YAMAHA SY
        
        
        // Construction du XML
        XmlElement bankList ("TABLE_DATA");
        
        //Construction du Headers
        // create an inner element..
        XmlElement* chien = new XmlElement ("HEADERS");
        
        // create an inner element..
        XmlElement* poule = new XmlElement ("COLUMN");
        
        poule->setAttribute ("columnId", 1);
        poule->setAttribute ("name", "ID");
        poule->setAttribute ("width", 120);
        
        chien->addChildElement (poule);
        // create an inner element..
        XmlElement* mpoule = new XmlElement ("COLUMN");
        
        mpoule->setAttribute ("columnId", 2);
        mpoule->setAttribute ("name", "BANK");
        mpoule->setAttribute ("width", 120);
        
        chien->addChildElement (mpoule);
        bankList.addChildElement (chien);
        
        
        //Construction des DATA
        XmlElement* xData = new XmlElement("DATA");
        
        for (int i = 0; i < numRows; ++i)
        {
            XmlElement* giraffe = new XmlElement ("ITEM");
            giraffe->setAttribute ("ID", i);
            giraffe->setAttribute ("BANK", arrayBank[i]);
            xData->addChildElement (giraffe);
        }
        bankList.addChildElement(xData);
        // dataList = &BankList;
        
        bankList.writeToFile(appDirPath.getFullPathName() + "/Bank.xml", "");

    }
    
        bool somethingIsBeingDraggedOver = false;
    ListBox sourceListBox  { "D+D source", nullptr };
    GroupComponent  groupDrop;
    SourceItemListboxContents sourceModel;
    TextEditor editText;

    int numRows;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankTableModel)
    };
    
    //==============================================================================
