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

        selectFirstBankAndReload();
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

    void mouseDown (const MouseEvent&) override
    {
        Logger::writeToLog("Bank Mouse event");
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
            return maxFiles;
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
                owner->reloadVoicesFromSelectedBankFile();
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
        
        void mouseDown (const MouseEvent&) override
        {
          Logger::writeToLog("BankTableModel -> clicked");
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
        Logger::writeToLog("fileDropped");
        Logger::writeToLog( "Files dropped: " + files.joinIntoString ("\n"));
        groupDrop.setVisible(false);
        File file(files.joinIntoString("\n"));
        String str = appDirPath.getFullPathName();
        str = str + "/";
        file.copyFileTo(str);
            Logger::writeToLog("FilesDropped: copy files OK");
 //       void MyClass::importAudioFile(File audioFile)
        File audioFile;
        for(int i = 0; i < files.size(); i++)
        {
            audioFile = files[i];
            if (audioFile.exists())
            {
                //create a new file based on the imported file's name and the path of the working directory
                File audioFileCopy (appDirPath.getFullPathName() + "/" + audioFile.getFileName());
                
                if (audioFileCopy.existsAsFile() == false) //if it doesn't yet exist...
                {
                    //...copy the imported audio file into the newly created file
                    audioFile.copyFileTo(audioFileCopy);
                }
                else if (audioFileCopy.existsAsFile() == true && audioFile.hasIdenticalContentTo(audioFileCopy) == false)
                    //if the file already exists (in terms of file name) but they aren't the same file in terms of content...
                {
                    //... copy the imported file but with different name
                    audioFileCopy = audioFileCopy.getNonexistentSibling();
                    audioFile.copyFileTo(audioFileCopy);
                }
                
            }
                //else, the imported file already exists in the directory so no new files need to be added
        }
         loadBank();
        repaint();
    }

//==============================================================================
    
    void loadBank()
    {
        arrayBank.clear();
        BankFiles.clear();
        if(!appDirPath.exists())
        {
            appDirPath.createDirectory();
            
            Logger::writeToLog("Creation du dossier de presets");
            Logger::writeToLog(appDirPath.getFullPathName());
            
        }
 
        //  appDirPath.findChildFiles(BankFiles, TypeOfFileToFind::findFiles, true, "someName");
        
        appDirPath.findChildFiles(BankFiles, File::TypesOfFileToFind::findFiles
                                  , false, "*.syx");
        numRows=BankFiles.size();
        
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
        
        for (int i = 0; i < numRows  ; ++i)
        {
            XmlElement* giraffe = new XmlElement ("ITEM");
            giraffe->setAttribute ("ID", i);
            giraffe->setAttribute ("BANK", BankFiles[i].getFileName());
            arrayBank.add( BankFiles[i].getFileName());
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
