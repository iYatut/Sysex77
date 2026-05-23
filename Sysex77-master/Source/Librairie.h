/*
 ==============================================================================
 
 Librairie.h
 Created: 13 Nov 2018 11:23:35pm
 Author:  Sébastien Portrait
 
 Notes:
 BANK limité à 64 PATCH pour pouvoir transferer la banque
 
 Objectifs:
 Se rapprocher de la Librairie a la "massive"
 [VOICES/MULTI/SEQ] [DUMP]
 [BANK] [PATCHES]
 
 *Lecture/Sauvegarde des fichiers SYX
 *Lecture/Sauvegarde des fichier MID
 
 Eventualitées
 *Ajout de TAG
 
 ==============================================================================
 */

#pragma once
#include <memory>

#include "BankTableModel.h"
#include "VoicesTableModel.h"
#include "Sy99BulkLibraryCapture.h"
#include "Sy99LibrarySyncEngine.h"
#include "Sy99DumpRequest.h"
#include "DeviceModel.h"
#include "MidiStreamLogger.h"

namespace LibrarySyncUi
{
    inline String txt (const char* utf8) noexcept
    {
        return String::fromUTF8 (utf8);
    }

    inline String syncTitle (int done, int total, bool fullLibrary) noexcept
    {
        const int pct = total > 0 ? jlimit (0, 100, (done * 100) / total) : 0;

        if (fullLibrary)
            return txt (u8"Полная синхронизация: ") + String (done) + " / " + String (total)
                   + "  (" + String (pct) + "%)";

        return txt (u8"Синхронизация: ") + String (done) + txt (u8" из ") + String (total)
               + "  (" + String (pct) + "%)";
    }
}


struct LibrairiePage   : public Component, public Button::Listener, private AsyncUpdater,
                         private Timer, public ChangeListener, public ChangeBroadcaster
{
    LibrairiePage()
      : autoSyncProgressBar (autoSyncProgressValue)
    {
        setOpaque(false);
        addBtAndMakeStyle(btVoice);
        addBtAndMakeStyle(btPreset1);
        btPreset1.setButtonText ("PRE1");
        addBtAndMakeStyle(btPreset2);
        btPreset2.setButtonText ("PRE2");
        addBtAndMakeStyle(btMulti);
        btMulti.setEnabled (true);
        addBtAndMakeStyle(btSeq);
        btSeq.setEnabled(false);
        btVoice.addListener (this);
        btPreset1.addListener (this);
        btPreset2.addListener (this);
        btMulti.addListener (this);
        addAndMakeVisible(labelInfoLine);
        labelInfoLine.setJustificationType (Justification::centredLeft);
        
        addChildComponent (labelAutoSyncBanner);
        labelAutoSyncBanner.setInterceptsMouseClicks (false, false);
        labelAutoSyncBanner.setJustificationType (Justification::centredLeft);
        labelAutoSyncBanner.setColour (Label::backgroundColourId, Colour (0xff1e3a1e));
        labelAutoSyncBanner.setColour (Label::textColourId, Colours::white);
        labelAutoSyncBanner.setFont (Font (15.0f, Font::bold));
        labelAutoSyncBanner.setMinimumHorizontalScale (0.65f);

        addChildComponent (labelAutoSyncDetail);
        labelAutoSyncDetail.setInterceptsMouseClicks (false, false);
        labelAutoSyncDetail.setJustificationType (Justification::centredLeft);
        labelAutoSyncDetail.setColour (Label::backgroundColourId, Colour (0xff1e3a1e));
        labelAutoSyncDetail.setColour (Label::textColourId, Colour (0xffb8ddb8));
        labelAutoSyncDetail.setFont (Font (13.0f));
        labelAutoSyncDetail.setMinimumHorizontalScale (0.65f);

        addChildComponent (autoSyncProgressBar);
        autoSyncProgressBar.setInterceptsMouseClicks (false, false);
        autoSyncProgressBar.setPercentageDisplay (false);
        autoSyncProgressBar.setColour (ProgressBar::backgroundColourId, Colour (0xff0d260d));
        autoSyncProgressBar.setColour (ProgressBar::foregroundColourId, Colour (0xff43a047));
        
        //    labelInfoLine.setJustificationType(Justification::centred);
        addAndMakeVisible(btSend);
        btSend.addListener(this);
        addAndMakeVisible(btReceive);
        addAndMakeVisible(btStop);
        btStop.setVisible(false);
        btStop.setColour(TextButton::ColourIds::buttonColourId, Colours::red);
        btStop.addListener(this);
        //    btReceive.setEnabled(false);
        btReceive.addListener(this);

        // --- Library skeleton (Prompt #12): IMPORT / EXPORT / SEND VOICE / REQUEST VOICE ---
        addAndMakeVisible (btImport);
        btImport.addListener (this);
        addAndMakeVisible (btExport);
        btExport.addListener (this);
        addAndMakeVisible (btSendVoice);
        btSendVoice.setTooltip ("Writes to SY99 internal memory. Clicking voices / Prev / Next only opens in the editor.");
        btSendVoice.addListener (this);
        addAndMakeVisible (btRequestVoice);
        btRequestVoice.addListener (this);

        addAndMakeVisible(bankList);
        bankList.addChangeListener(this);
        
        addAndMakeVisible(voicesListA);
        addAndMakeVisible(voicesListB);
        addAndMakeVisible(voicesListC);
        addAndMakeVisible(voicesListD);

        libraryVoiceHighlightCallback() = [this] (int globalIdx)
        {
            highlightLibraryVoiceSlot (globalIdx);
        };

        librarySlotWithEditorCallback() = [this] (Sy99LibraryContentPage page, int mm)
        {
            auto openSlot = [page, mm]
            {
                if (sy99ShouldUseSy99SlotEditor())
                {
                    selectLibrarySlotWithEditor (page, mm);
                    return;
                }

                if (libraryPageIsMulti (page))
                    selectLibraryPageSlotExclusive (mm / 16, mm % 16);
                else
                    selectLibraryVoiceSlot (mm % 16, (mm / 16) * 16);
            };

            if (libraryPageIsMulti (page) || libraryVoiceSuppressProgramChangeSend())
                openSlot();
            else
                tryLeaveEditorContext (openSlot);
        };

        librarySynthPanelNavigateCallback() = [this] (Sy99LibraryContentPage page, int mm)
        {
            if (libraryContentPage() != page)
                switchLibraryContentPage (page);

            libraryVoiceSuppressProgramChangeSend() = true;

            if (sy99ShouldUseSy99SlotEditor())
            {
                selectLibrarySlotWithEditor (page, mm);
                libraryVoiceSuppressProgramChangeSend() = false;
                return;
            }

            if (libraryPageIsMulti (page))
                selectLibraryPageSlotExclusive (0, mm);
            else
            {
                const int bankIdx = mm / 16;
                const int rowInBank = mm % 16;
                selectLibraryPageSlotExclusive (bankIdx, rowInBank);
            }

            libraryVoiceSuppressProgramChangeSend() = false;
        };

        libraryVoiceOpenedCallback() = [this] (int globalIdx, const String& voiceName)
        {
            labelInfoLine.setText ("Synced: \"" + voiceName.trimEnd()
                                   + "\" (" + libraryVoiceSy99SlotCode (globalIdx) + ")",
                                   dontSendNotification);
        };

        auto& syncHost = sy99LibrarySyncHost();
        syncHost.requestUiRefresh = [this] { triggerAsyncUpdate(); };
        syncHost.preparePhaseOnUi = [this] (int phaseIndex)
        {
            prepareLibrarySyncPhase (sy99FullLibrarySyncPhaseAt (phaseIndex));
            relayoutSyncVoiceListsLight();
            triggerAsyncUpdate();
        };
        syncHost.finishFullSyncOnUi = [this] { finishFullLibrarySyncOnUi(); };
        syncHost.finishAuto64OnUi = [this] (const File& saved) { finishAutoSync64OnUi (saved); };
        syncHost.failAuto64OnUi = [this] { failAutoSync64OnUi(); };

        btVoice.setToggleState(true, NotificationType::dontSendNotification);
        
        labelInfoLine.setText (LibrarySyncUi::txt (u8"Выберите банк слева, IMPORT или синхронизацию"),
                               dontSendNotification);
        
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");

        if (sy99LibrarySession().active)
        {
            switchLibraryContentPage (libraryContentPage());
            sy99RestoreLibrarySlotFromPersistence();
        }
        else
        {
            labelInfoLine.setText (LibrarySyncUi::txt (u8"Запустите синхронизацию для загрузки библиотеки SY-99"),
                                   dontSendNotification);
        }
    }
    ~LibrairiePage()
    {
        btSend.removeListener(this);
        btStop.removeListener(this);
        btReceive.removeListener(this);
        btImport.removeListener(this);
        btExport.removeListener(this);
        btSendVoice.removeListener(this);
        btRequestVoice.removeListener(this);
        btVoice.removeListener (this);
        btPreset1.removeListener (this);
        btPreset2.removeListener (this);
        btMulti.removeListener (this);
        sy99LibrarySyncHost() = Sy99LibrarySyncHost{};
        bankList.removeChangeListener(this);
        stopTimer();
        
    }
    void mouseDown (const MouseEvent& e) override
    {
        Logger::writeToLog("Librairie mouse event");
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog ("Librairie: changeListener");
        refreshAutoSyncVoiceLists();

        if (arrayBank.isEmpty())
        {
            bankSelectedVoiceIndex = -1;
            bankSelectedVoiceName.clear();
            switchLibraryContentPage (Sy99LibraryContentPage::internalVoices);
            labelInfoLine.setText (LibrarySyncUi::txt (u8"Библиотека пуста — выберите .syx слева, IMPORT или синхронизацию"),
                                   dontSendNotification);
        }
        else if (bankSelected.isEmpty() || arrayListVoices.isEmpty())
        {
            bankSelectedVoiceIndex = -1;
            bankSelectedVoiceName.clear();
            labelInfoLine.setText (LibrarySyncUi::txt (u8"Выберите банк слева или запустите синхронизацию"),
                                   dontSendNotification);
        }
        else
        {
            labelInfoLine.setText (bankSelected + " — "
                                   + String (arrayListVoices.size())
                                   + LibrarySyncUi::txt (u8" голосов"),
                                   dontSendNotification);
        }

        repaint();
    }
    void resized() override
    {
        // const int bankWidth = 140; // Largeur de la table BANK
        int tableWidth;
        tableWidth = (getWidth()-  64)/5;
        
        btVoice.setBounds (tableWidth + 20, 10, 52, 24);
        btPreset1.setBounds (tableWidth + 76, 10, 52, 24);
        btPreset2.setBounds (tableWidth + 132, 10, 52, 24);
        btMulti.setBounds (tableWidth + 188, 10, 52, 24);

        btSendVoice.setBounds   (tableWidth + 244, 10, 96, 24);
        btRequestVoice.setBounds(tableWidth + 344, 10, 96, 24);
        btImport.setBounds      (tableWidth + 444, 10, 64, 24);
        btExport.setBounds      (tableWidth + 512, 10, 64, 24);

        btSend.setBounds (getWidth()-74,10,64,24);
        btReceive.setBounds(getWidth()-152, 10, 74, 24);
        btStop.setBounds(btReceive.getBounds());

        const bool syncPanel = labelAutoSyncBanner.isVisible();
        const int syncPanelTop = 36;
        const int syncPanelH = syncPanel ? 58 : 0;
        const int syncPanelW = juce::jmax (200, getWidth() - tableWidth - 24);
        const int bottomBarH = 24;
        const int voicesTop = syncPanel ? (syncPanelTop + syncPanelH + 6) : 44;
        const int contentBottom = getHeight() - bottomBarH - 6;
        const int voicesH = juce::jmax (80, contentBottom - voicesTop);
        const int gridLeft = tableWidth + 16;
        const int gridW = tableWidth * 4 + 30;

        if (syncPanel)
        {
            labelAutoSyncBanner.setBounds (tableWidth + 16, syncPanelTop, syncPanelW, 22);
            labelAutoSyncDetail.setBounds (tableWidth + 16, syncPanelTop + 22, syncPanelW, 18);
            autoSyncProgressBar.setBounds (tableWidth + 16, syncPanelTop + 42, syncPanelW, 14);
        }

        voicesListA.setBounds (tableWidth + 16, voicesTop, tableWidth, voicesH);
        voicesListB.setBounds (tableWidth + 26 + tableWidth, voicesTop, tableWidth, voicesH);
        voicesListC.setBounds (tableWidth + 36 + tableWidth + tableWidth, voicesTop, tableWidth, voicesH);
        voicesListD.setBounds (tableWidth + 46 + tableWidth + tableWidth + tableWidth, voicesTop, tableWidth, voicesH);

        const bool multiLayout = libraryPageIsMulti (libraryContentPage());
        voicesListB.setVisible (! multiLayout);
        voicesListC.setVisible (! multiLayout);
        voicesListD.setVisible (! multiLayout);

        if (multiLayout)
        {
            voicesListA.setBounds (gridLeft, voicesTop, gridW, voicesH);
        }

        voicesListA.resized();
        voicesListB.resized();
        voicesListC.resized();
        voicesListD.resized();

        bankList.setBounds(8, 10, tableWidth, getHeight()-20);
        labelInfoLine.setBounds (8, getHeight() - bottomBarH, getWidth() - 16, bottomBarH - 2);
        
    }

    void applySyncPanelText (const String& title, const String& detail) noexcept
    {
        labelAutoSyncBanner.setText (title, dontSendNotification);
        labelAutoSyncDetail.setText (detail, dontSendNotification);
        labelInfoLine.setText (detail.isNotEmpty() ? detail : title, dontSendNotification);
    }

    void setAutoSyncPanelVisible (bool visible) noexcept
    {
        labelAutoSyncBanner.setVisible (visible);
        labelAutoSyncDetail.setVisible (visible);
        autoSyncProgressBar.setVisible (visible);

        if (visible)
        {
            labelAutoSyncBanner.toFront (false);
            labelAutoSyncDetail.toFront (false);
            autoSyncProgressBar.toFront (false);
        }

        resized();
        repaint();
    }

    void refreshAutoSyncVoiceLists() noexcept
    {
        voicesListA.loadBank();
        voicesListB.loadBank();
        voicesListC.loadBank();
        voicesListD.loadBank();
        voicesListA.resized();
        voicesListB.resized();
        voicesListC.resized();
        voicesListD.resized();
        voicesListA.repaint();
        voicesListB.repaint();
        voicesListC.repaint();
        voicesListD.repaint();

        for (auto* lb : { &voicesListA.sourceListBox, &voicesListB.sourceListBox,
                          &voicesListC.sourceListBox, &voicesListD.sourceListBox })
            if (auto* header = lb->getHeaderComponent())
                header->repaint();
    }

    static constexpr int kSyncProgressUiEverySlots = 4;
    static constexpr int kSyncGridRefreshEverySlots = 16;
    static constexpr int kSyncPartialRxUiMinMs = 400;

    void setLibraryContentPageSilent (Sy99LibraryContentPage page) noexcept
    {
        libraryContentPage() = page;
        resetLibraryRecallContext();
        highlightLibraryVoiceSlot (-1);

        btVoice.setToggleState (page == Sy99LibraryContentPage::internalVoices, dontSendNotification);
        btPreset1.setToggleState (page == Sy99LibraryContentPage::preset1Voices, dontSendNotification);
        btPreset2.setToggleState (page == Sy99LibraryContentPage::preset2Voices, dontSendNotification);
        btMulti.setToggleState (page == Sy99LibraryContentPage::multiInternal
                                || page == Sy99LibraryContentPage::multiPreset,
                                dontSendNotification);

        for (auto* lb : { &voicesListA.sourceListBox, &voicesListB.sourceListBox,
                          &voicesListC.sourceListBox, &voicesListD.sourceListBox })
            lb->updateContent();

        voicesListA.resized();
        voicesListB.resized();
        voicesListC.resized();
        voicesListD.resized();
    }

    void relayoutSyncVoiceListsLight() noexcept
    {
        voicesListA.sourceListBox.updateContent();
        voicesListA.resized();
        voicesListB.sourceListBox.updateContent();
        voicesListB.resized();
        voicesListC.sourceListBox.updateContent();
        voicesListC.resized();
        voicesListD.sourceListBox.updateContent();
        voicesListD.resized();
    }

    void repaintSyncVoiceGrid (int highlightGlobalMm) noexcept
    {
        if (highlightGlobalMm >= 0)
            highlightLibraryVoiceSlot (highlightGlobalMm);

        voicesListA.sourceListBox.repaint();
        voicesListB.sourceListBox.repaint();
        voicesListC.sourceListBox.repaint();
        voicesListD.sourceListBox.repaint();
    }

    /** Progress bar always; grid repaint on a stable cadence (not every slot). */
    void tickLibrarySyncProgressUi (const String& phaseDetail,
                                    bool forceGridRefresh = false) noexcept
    {
        if (sy99FullLibrarySyncSession().active)
        {
            auto& s = sy99FullLibrarySyncSession();
            const int total = sy99FullLibrarySyncTotalRequests();
            autoSyncProgressValue = total > 0 ? (double) s.requestsCompleted / (double) total : 0.0;

            const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);
            String detail = phaseDetail.isNotEmpty()
                                ? phaseDetail
                                : sy99FullLibrarySyncPhaseLabel (s.phaseIndex, s.currentMm);

            if (phase.updatesVoicePreview && phaseDetail.isEmpty() && s.currentMm >= 0)
            {
                auto& slots = libraryPageSlotNames (libraryContentPageFromId (phase.libraryPage));

                if (s.currentMm < slots.size())
                {
                    const String name = slots[s.currentMm].trimEnd();

                    if (name.isNotEmpty())
                        detail += "  \"" + name + "\"";
                }
            }

            applySyncPanelText (LibrarySyncUi::syncTitle (s.requestsCompleted, total, true), detail);

            const bool gridTick = forceGridRefresh
                                  || (s.requestsCompleted > 0
                                      && s.requestsCompleted % kSyncGridRefreshEverySlots == 0);

            if (gridTick && phase.updatesVoicePreview)
                repaintSyncVoiceGrid (s.currentMm);

            autoSyncProgressBar.repaint();
            return;
        }

        auto& s = sy99AutoSync64Session();

        if (! s.active)
            return;

        const int total = kSy99AutoSyncInternalVoiceCount;
        autoSyncProgressValue = total > 0 ? (double) s.loadedCount / (double) total : 0.0;

        String detail = phaseDetail;

        if (detail.isEmpty() && s.currentMm >= 0)
        {
            auto& slots = arrayListVoices;

            if (s.currentMm < slots.size())
            {
                const String name = slots[s.currentMm].trimEnd();

                if (name.isNotEmpty())
                    detail = sy99AutoSync64SlotLabel (s.currentMm) + "  \"" + name + "\"";
            }
        }

        applySyncPanelText (LibrarySyncUi::syncTitle (s.loadedCount, total, false), detail);

        const bool gridTick = forceGridRefresh
                              || (s.loadedCount > 0
                                  && s.loadedCount % kSyncGridRefreshEverySlots == 0);

        if (gridTick)
            repaintSyncVoiceGrid (s.currentMm);

        autoSyncProgressBar.repaint();
    }

    void maybeTickLibrarySyncProgressUi (const String& phaseDetail = {}) noexcept
    {
        if (sy99FullLibrarySyncSession().active)
        {
            const auto& s = sy99FullLibrarySyncSession();
            const bool force = phaseDetail.isNotEmpty();
            const bool periodic = s.requestsCompleted > 0
                                  && (s.requestsCompleted % kSyncProgressUiEverySlots == 0);

            if (force || periodic)
                tickLibrarySyncProgressUi (phaseDetail, force);
            return;
        }

        if (sy99AutoSync64Session().active)
        {
            const auto& s = sy99AutoSync64Session();
            const bool force = phaseDetail.isNotEmpty();
            const bool periodic = s.loadedCount > 0
                                  && (s.loadedCount % kSyncProgressUiEverySlots == 0);

            if (force || periodic)
                tickLibrarySyncProgressUi (phaseDetail, force);
        }
    }

    void switchLibraryContentPage (Sy99LibraryContentPage page) noexcept
    {
        setLibraryContentPageSilent (page);
        resetLibraryRecallContext();

        if (sy99LibrarySession().active && sy99IsSy99LibrarySessionBankLabel (bankSelected))
            sy99EnsureLibraryPageSlotNames (page);

        refreshAutoSyncVoiceLists();
        resized();
        repaint();

        if (sy99LibrarySession().active)
            librarySavePersistedSelectionToDisk();
    }

    void prepareLibrarySyncPhase (const Sy99LibrarySyncPhase& phase) noexcept
    {
        if (! phase.switchesLibraryPage)
            return;

        const auto page = libraryContentPageFromId (phase.libraryPage);

        if (sy99AnyLibrarySyncActive())
            setLibraryContentPageSilent (page);
        else
            switchLibraryContentPage (page);

        if (phase.updatesVoicePreview)
            beginLibraryPagePreview (page, phase.mmCount);
    }

    void updateAutoSyncProgressUi (const String& phaseDetail = {}) noexcept
    {
        if (sy99FullLibrarySyncSession().active)
        {
            tickLibrarySyncProgressUi (phaseDetail, phaseDetail.isNotEmpty());
            return;
        }

        if (! sy99AutoSync64Session().active)
            return;

        tickLibrarySyncProgressUi (phaseDetail, phaseDetail.isNotEmpty());
    }

    void onAutoSyncVoiceReceived (int mm, const Array<MidiMessage>& batch) noexcept
    {
        auto& s = sy99AutoSync64Session();
        updateAutoSyncVoicePreviewSlot (mm, sy99AutoSyncVoiceNameFromMessages (batch));
        s.loadedCount = s.accumulator.size();
    }

    void updateFullLibrarySyncProgressUi (const String& phaseDetail = {}) noexcept
    {
        tickLibrarySyncProgressUi (phaseDetail, phaseDetail.isNotEmpty());
    }

    void onFullLibrarySyncResponse (int mm, const Array<MidiMessage>& batch) noexcept
    {
        juce::ignoreUnused (mm, batch);
    }

    void handleAsyncUpdate() override
    {
        refreshLibrarySyncUiLight();
    }

    void repaintOneSyncVoiceSlot (int globalMm) noexcept
    {
        if (globalMm < 0)
            return;

        highlightLibraryVoiceSlot (globalMm);

        const int rowsPerBank = libraryPageRowsPerBank();
        const int bankIdx = globalMm / rowsPerBank;

        switch (bankIdx)
        {
            case 0:
                voicesListA.sourceListBox.updateContent();
                voicesListA.sourceListBox.repaint();
                break;
            case 1:
                voicesListB.sourceListBox.updateContent();
                voicesListB.sourceListBox.repaint();
                break;
            case 2:
                voicesListC.sourceListBox.updateContent();
                voicesListC.sourceListBox.repaint();
                break;
            case 3:
                voicesListD.sourceListBox.updateContent();
                voicesListD.sourceListBox.repaint();
                break;
            default: break;
        }
    }

    /** Coalesced UI tick: progress bar + one list row (no per-slot callAsync queue). */
    void refreshLibrarySyncUiLight() noexcept
    {
        if (sy99FullLibrarySyncSession().active)
        {
            auto& s = sy99FullLibrarySyncSession();
            const int total = sy99FullLibrarySyncTotalRequests();
            autoSyncProgressValue = total > 0 ? (double) s.requestsCompleted / (double) total : 0.0;

            const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);
            const int showMm = s.currentMm;
            String detail = sy99FullLibrarySyncPhaseLabel (s.phaseIndex, showMm);

            const bool fullGrid = phase.updatesVoicePreview
                                  && s.requestsCompleted > 0
                                  && (s.requestsCompleted % kSyncGridRefreshEverySlots == 0);
            if (phase.updatesVoicePreview)
            {
                String slotName;

                {
                    const juce::ScopedLock sl (sy99LibrarySyncPreviewLock());
                    auto& slots = libraryPageSlotNames (libraryContentPageFromId (phase.libraryPage));

                    if (showMm < slots.size())
                        slotName = slots[showMm].trimEnd();
                }

                if (slotName.isNotEmpty())
                    detail += "  \"" + slotName + "\"";

                if (fullGrid)
                    relayoutSyncVoiceListsLight();
                else
                    repaintOneSyncVoiceSlot (showMm);
            }

            applySyncPanelText (LibrarySyncUi::syncTitle (s.requestsCompleted, total, true), detail);
            autoSyncProgressBar.repaint();
            return;
        }

        auto& s = sy99AutoSync64Session();

        if (! s.active)
            return;

        const int total = kSy99AutoSyncInternalVoiceCount;
        autoSyncProgressValue = total > 0 ? (double) s.loadedCount / (double) total : 0.0;

        const int showMm = s.currentMm;
        String detail = sy99AutoSync64SlotLabel (showMm);
        String slotName;

        {
            const juce::ScopedLock sl (sy99LibrarySyncPreviewLock());
            auto& slots = libraryPageSlotNames (Sy99LibraryContentPage::internalVoices);

            if (showMm < slots.size())
                slotName = slots[showMm].trimEnd();
        }

        if (slotName.isNotEmpty())
            detail += "  \"" + slotName + "\"";

        applySyncPanelText (LibrarySyncUi::syncTitle (s.loadedCount, total, false), detail);
        repaintOneSyncVoiceSlot (showMm);
        autoSyncProgressBar.repaint();
    }

    void finishFullLibrarySyncOnUi() noexcept
    {
        auto& s = sy99FullLibrarySyncSession();
        bulkSessionSaveCount += s.savedFiles.size();

        if (bankList.activateSy99LibrarySession())
        {
            sy99RefreshSy99LibraryNamesFromSession (s.includeExtendedPhases);
            refreshAutoSyncVoiceLists();
            switchLibraryContentPage (Sy99LibraryContentPage::internalVoices);
            sy99RestoreLibrarySlotFromPersistence();
        }
        else
            bankList.reloadAfterLibraryFileAdded();

        applySyncPanelText (LibrarySyncUi::txt (u8"Полная синхронизация завершена"),
                            String (s.savedFiles.size()) + LibrarySyncUi::txt (u8" файлов .syx"));
        labelInfoLine.setText (LibrarySyncUi::txt (u8"Полная синхронизация завершена: ")
                               + String (s.requestsCompleted) + " / "
                               + String (sy99FullLibrarySyncTotalRequests())
                               + LibrarySyncUi::txt (u8" — ")
                               + String (s.savedFiles.size())
                               + LibrarySyncUi::txt (u8" .syx в captures/"),
                               dontSendNotification);
        Logger::writeToLog ("[FullLibrarySync] Complete — "
                            + String (s.savedFiles.size()) + " files saved");
        sy99AuditFullLibrarySyncCaptures (s.savedFiles, s.includeExtendedPhases);
        relayoutSyncVoiceListsLight();
        endFullLibrarySyncSession (true);
    }

    void finishAutoSync64OnUi (const File& saved) noexcept
    {
        auto& s = sy99AutoSync64Session();

        if (saved.existsAsFile())
        {
            ++bulkSessionSaveCount;

            if (bankList.activateSy99LibrarySession())
            {
                refreshAutoSyncVoiceLists();
                switchLibraryContentPage (Sy99LibraryContentPage::internalVoices);
                sy99RestoreLibrarySlotFromPersistence();
            }
            else
                bankList.selectBankFileAndReloadVoices (saved);

            applySyncPanelText (LibrarySyncUi::txt (u8"Синхронизация завершена"),
                                saved.getFileName());
            labelInfoLine.setText (LibrarySyncUi::txt (u8"Синхронизация завершена: загружено ")
                                   + String (s.loadedCount) + LibrarySyncUi::txt (u8" из ")
                                   + String (kSy99AutoSyncInternalVoiceCount)
                                   + " — " + saved.getFileName(),
                                   dontSendNotification);
            Logger::writeToLog ("[AutoSync64] Saved " + saved.getFullPathName());
            endAutoSync64Session (true);
        }
        else
        {
            endAutoSync64Session (false);
            labelInfoLine.setText (LibrarySyncUi::txt (u8"Синхронизация: не удалось записать .syx"),
                                   dontSendNotification);
        }
    }

    void failAutoSync64OnUi() noexcept
    {
        const int mm = sy99AutoSync64Session().currentMm;
        endAutoSync64Session (false);
        labelInfoLine.setText (LibrarySyncUi::txt (u8"Синхронизация остановлена на ")
                               + sy99AutoSync64SlotLabel (mm)
                               + LibrarySyncUi::txt (u8" — проверьте Bulk Protect, Device ID, MIDI"),
                               dontSendNotification);
        Logger::writeToLog ("[AutoSync64] Failed mm=" + String::toHexString (mm));
    }

    //==============================================================================
    void timerCallback() override
    {
        if (sy99AnyLibrarySyncActive())
            return;

        ++timeOut;

        if (requestSysex && ! arraySysex.isEmpty())
        {
            Sy99BulkDumpAnalysis preview;
            preview = analyzeCapturedSysexMessages (arraySysex);
            labelInfoLine.setText ("Receiving… " + preview.makeSummaryLine()
                                   + " (STOP when SY99 shows Completed)",
                                   dontSendNotification);
        }

        const int threshold = bulkCaptureIdleThresholdTicks (arraySysex.size());

        if (timeOut <= threshold)
            return;

        if (arraySysex.isEmpty())
        {
            if (bulkCapturePendingDumpRequest
                && ! bulkCaptureSym78101Request
                && bulkCaptureDumpRequestTailVariant == sy99DumpRequestTailMtMmChecksum
                && ! bulkCaptureDumpRequestRetried)
            {
                bulkCaptureDumpRequestTailVariant = sy99DumpRequestTailHeaderOnly;
                bulkCaptureDumpRequestRetried = true;
                timeOut = 0;
                sendPendingDumpRequest();
                labelInfoLine.setText ("No reply to variant A — retrying dump request variant B…",
                                       dontSendNotification);
                Logger::writeToLog ("[DumpReq] Timeout variant A — retry variant B case="
                                    + bulkCaptureDumpRequestCaseTag);
                return;
            }

            if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous)
            {
                timeOut = 0;
                return;
            }

            endBulkCaptureSession (false);
            labelInfoLine.setText ("Dump: no SysEx received — check MIDI in + Bulk Protect",
                                   dontSendNotification);
            return;
        }

        commitBufferedDumpToLibrary();

        if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous)
        {
            timeOut = 0;
            labelInfoLine.setText ("Saved " + String (bulkSessionSaveCount) + " file(s). "
                                   + bulkCaptureInstructionForKind (bulkCaptureKind),
                                   dontSendNotification);
            return;
        }

        endBulkCaptureSession (true);
    }

    void beginBulkCapture (Sy99BulkCaptureKind kind,
                           Sy99BulkCaptureSessionMode mode) noexcept
    {
        bulkCaptureKind = kind;
        bulkSessionMode = mode;
        bulkSessionSaveCount = 0;

        arraySysex.clear();
        newMessage = false;
        requestSysex = true;
        timeOut = 0;

        btReceive.setEnabled (false);
        btRequestVoice.setEnabled (false);
        btSend.setEnabled (false);
        btStop.setVisible (true);
        startTimer (kSy99LibrarySyncTimerIntervalMs);

        labelInfoLine.setText (bulkCaptureInstructionForKind (kind), dontSendNotification);
        Logger::writeToLog ("[BulkRecv] Started kind="
                            + String ((int) kind)
                            + " mode=" + String ((int) mode));
    }

    bool commitBufferedDumpToLibrary() noexcept
    {
        if (arraySysex.isEmpty() || ! arraySysex[0].isSysEx())
            return false;

        Sy99BulkDumpAnalysis analysis;
        const auto preview = analyzeCapturedSysexMessages (arraySysex);
        String prefix;

        if (bulkCaptureDumpRequestCaseTag.isNotEmpty())
            prefix = "REQTEST-" + bulkCaptureDumpRequestCaseTag;
        else if (bulkCaptureKind == Sy99BulkCaptureKind::voiceSingle)
            prefix = "VOICE1";
        else
            prefix = preview.suggestFileBaseName();

        const File saved = saveCapturedSysexToLibrary (arraySysex, prefix, &analysis);
        arraySysex.clear();

        if (! saved.existsAsFile())
            return false;

        ++bulkSessionSaveCount;
        bankList.selectBankFileAndReloadVoices (saved);

        labelInfoLine.setText ("Saved: " + saved.getFileName()
                               + " (" + analysis.makeSummaryLine() + ")",
                               dontSendNotification);
        return true;
    }

    void endBulkCaptureSession (bool hadData) noexcept
    {
        const bool wasDumpRequest = bulkCapturePendingDumpRequest;
        const String dumpCaseTag = bulkCaptureDumpRequestCaseTag;
        const bool dumpRetried = bulkCaptureDumpRequestRetried;

        stopTimer();
        requestSysex = false;
        newMessage = false;
        timeOut = 0;
        arraySysex.clear();
        clearDumpRequestCaptureState();

        btReceive.setEnabled (true);
        btRequestVoice.setEnabled (true);
        btSend.setEnabled (true);
        btStop.setVisible (false);

        if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous && bulkSessionSaveCount > 0)
        {
            labelInfoLine.setText ("Receive finished: " + String (bulkSessionSaveCount)
                                   + " file(s) in library",
                                   dontSendNotification);
        }
        else if (wasDumpRequest && bulkSessionSaveCount == 0)
        {
            labelInfoLine.setText ("SY99 не ответил на dump request (case " + dumpCaseTag
                                   + ", variants A"
                                   + (dumpRetried ? "+B" : String())
                                   + "). Bulk Protect=OFF, Engine=01, MIDI In+Out открыты. "
                                   + "Смотрите [TX] в MIDI monitor и [DumpReq] в логе.",
                                   dontSendNotification);
        }
        else if (! hadData && bulkSessionSaveCount == 0)
        {
            juce::ignoreUnused (hadData);
        }

        Logger::writeToLog ("[BulkRecv] Session ended, saved="
                            + String (bulkSessionSaveCount));
    }

    void sendPendingDumpRequest() noexcept
    {
        if (bulkCaptureSym78101Request)
        {
            sendSym78101BulkRequest (DeviceModel::getInstance().getSysExDeviceID(),
                                     bulkCaptureSym78101LmType.toRawUTF8(),
                                     bulkCaptureSym78101Byte28,
                                     bulkCaptureDumpRequestMm);
            return;
        }

        sendSy99VoiceDumpRequest (DeviceModel::getInstance().getSysExDeviceID(),
                                  bulkCaptureDumpRequestMt,
                                  bulkCaptureDumpRequestMm,
                                  bulkCaptureDumpRequestTailVariant);
    }

    void beginBulkCaptureWithSym78101Request (const String& caseTag,
                                              const char* lmType6,
                                              uint8 memoryNumber,
                                              uint8 byte28 = 0) noexcept
    {
        if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
        {
            const String err = portStatus();

            if (err.isNotEmpty())
            {
                labelInfoLine.setText (err, dontSendNotification);
                Logger::writeToLog ("[DumpReq] Blocked case=" + caseTag + ": " + err);
                return;
            }
        }

        bulkCapturePendingDumpRequest = true;
        bulkCaptureSym78101Request = true;
        bulkCaptureSym78101LmType = String (lmType6 != nullptr ? lmType6 : "8101VC");
        bulkCaptureSym78101Byte28 = byte28;
        bulkCaptureDumpRequestCaseTag = caseTag;
        bulkCaptureDumpRequestMt = 0;
        bulkCaptureDumpRequestMm = memoryNumber;
        bulkCaptureDumpRequestTailVariant = sy99DumpRequestTailMtMmChecksum;
        bulkCaptureDumpRequestRetried = false;

        beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                          Sy99BulkCaptureSessionMode::singleAutoClose);
        sendPendingDumpRequest();

        labelInfoLine.setText ("8101 dump request sent (case " + caseTag
                               + ") — смотрите [TX] в MIDI monitor, ждём ответ SY99…",
                               dontSendNotification);
    }

    void beginBulkCaptureWithDumpRequest (const String& caseTag,
                                          uint8 memoryType,
                                          uint8 memoryNumber) noexcept
    {
        if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
        {
            const String err = portStatus();

            if (err.isNotEmpty())
            {
                labelInfoLine.setText (err, dontSendNotification);
                Logger::writeToLog ("[DumpReq] Blocked case=" + caseTag + ": " + err);
                return;
            }
        }

        bulkCapturePendingDumpRequest = true;
        bulkCaptureSym78101Request = false;
        bulkCaptureSym78101LmType.clear();
        bulkCaptureSym78101Byte28 = 0;
        bulkCaptureDumpRequestCaseTag = caseTag;
        bulkCaptureDumpRequestMt = memoryType;
        bulkCaptureDumpRequestMm = memoryNumber;
        bulkCaptureDumpRequestTailVariant = sy99DumpRequestTailMtMmChecksum;
        bulkCaptureDumpRequestRetried = false;

        beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                          Sy99BulkCaptureSessionMode::singleAutoClose);
        sendPendingDumpRequest();

        labelInfoLine.setText ("Dump request variant A sent (case " + caseTag
                               + ") — смотрите [TX] в MIDI monitor, ждём ответ SY99…",
                               dontSendNotification);
    }

    void clearDumpRequestCaptureState() noexcept
    {
        bulkCapturePendingDumpRequest = false;
        bulkCaptureSym78101Request = false;
        bulkCaptureSym78101LmType.clear();
        bulkCaptureSym78101Byte28 = 0;
        bulkCaptureDumpRequestCaseTag.clear();
        bulkCaptureDumpRequestMt = 0;
        bulkCaptureDumpRequestMm = 0;
        bulkCaptureDumpRequestTailVariant = sy99DumpRequestTailMtMmChecksum;
        bulkCaptureDumpRequestRetried = false;
    }

    void beginAutoSync64InternalVoices() noexcept
    {
        if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
        {
            const String err = portStatus();

            if (err.isNotEmpty())
            {
                labelInfoLine.setText (err, dontSendNotification);
                Logger::writeToLog ("[AutoSync64] Blocked: " + err);
                return;
            }
        }

        sy99AutoSync64Reset();
        sy99AutoSync64Session().active = true;
        suppressEditorExitPrompt() = true;
        beginAutoSyncVoicePreview();

        bulkCaptureKind = Sy99BulkCaptureKind::voiceBank64;
        bulkSessionMode = Sy99BulkCaptureSessionMode::singleAutoClose;
        bulkSessionSaveCount = 0;
        clearDumpRequestCaptureState();

        Sy99MidiTransport::instance().clearReceiveBuffer();
        arraySysex.clear();
        newMessage = false;
        requestSysex = true;
        timeOut = 0;

        btReceive.setEnabled (false);
        btRequestVoice.setEnabled (false);
        btSend.setEnabled (false);
        btStop.setVisible (true);
        startTimer (kSy99LibrarySyncTimerIntervalMs);

        setAutoSyncPanelVisible (true);
        autoSyncProgressValue = 0.0;
        Sy99LibrarySyncEngine::instance().ensureStarted();
        sy99LibrarySyncSendAuto64Request();
        triggerAsyncUpdate();
        Logger::writeToLog ("[AutoSync64] Started — 64 programmatic voice requests");
    }

    void endAutoSync64Session (bool success) noexcept
    {
        suppressEditorExitPrompt() = false;
        setAutoSyncPanelVisible (false);
        sy99AutoSync64Reset();
        endBulkCaptureSession (success);
    }

    void beginFullLibraryStartupSync (bool includeExtendedPhases = false) noexcept
    {
        if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
        {
            const String err = portStatus();

            if (err.isNotEmpty())
            {
                labelInfoLine.setText (err, dontSendNotification);
                Logger::writeToLog ("[FullLibrarySync] Blocked: " + err);
                return;
            }
        }

        sy99AutoSync64Reset();
        sy99FullLibrarySyncReset();
        sy99FullLibrarySyncSession().active = true;
        sy99FullLibrarySyncSession().includeExtendedPhases = includeExtendedPhases;
        suppressEditorExitPrompt() = true;
        const int firstPhase = sy99LibrarySyncFirstPhaseIndex (includeExtendedPhases);
        sy99FullLibrarySyncSession().phaseIndex = firstPhase;
        prepareLibrarySyncPhase (sy99FullLibrarySyncPhaseAt (firstPhase));

        bulkCaptureKind = Sy99BulkCaptureKind::receiveAll;
        bulkSessionMode = Sy99BulkCaptureSessionMode::singleAutoClose;
        bulkSessionSaveCount = 0;
        clearDumpRequestCaptureState();

        Sy99MidiTransport::instance().clearReceiveBuffer();
        arraySysex.clear();
        newMessage = false;
        requestSysex = true;
        timeOut = 0;

        btReceive.setEnabled (false);
        btRequestVoice.setEnabled (false);
        btSend.setEnabled (false);
        btStop.setVisible (true);
        startTimer (kSy99LibrarySyncTimerIntervalMs);

        setAutoSyncPanelVisible (true);
        autoSyncProgressValue = 0.0;
        Sy99LibrarySyncEngine::instance().ensureStarted();
        sy99LibrarySyncSendFullRequest();
        triggerAsyncUpdate();
        Logger::writeToLog ("[FullLibrarySync] Started — "
                            + String (sy99FullLibrarySyncTotalRequests()) + " requests, "
                            + String (sy99FullLibrarySyncEffectivePhaseCount()) + " phases"
                            + (includeExtendedPhases ? " (extended)" : " (fast)"));
    }

    void endFullLibrarySyncSession (bool success) noexcept
    {
        suppressEditorExitPrompt() = false;
        setAutoSyncPanelVisible (false);
        sy99FullLibrarySyncReset();
        endBulkCaptureSession (success);
    }

    void saveFullLibrarySyncCurrentPhase() noexcept
    {
        auto& s = sy99FullLibrarySyncSession();

        if (s.phaseIndex < 0 || s.phaseIndex >= sy99FullLibrarySyncPhaseCount())
            return;

        if (s.phaseAccumulator.isEmpty())
            return;

        const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);
        const File saved = saveFullLibrarySyncPhaseCapture (s.phaseAccumulator, phase.filePrefix);

        if (saved.existsAsFile())
        {
            ++bulkSessionSaveCount;
            s.savedFiles.add (saved);
            Logger::writeToLog ("[FullLibrarySync] Saved phase " + String (s.phaseIndex)
                                + " " + saved.getFullPathName());
        }
    }

    void showDumpRequestVerifyMenu()
    {
        PopupMenu menu;
        // TODO: edit-buffer 8101 request format — SYM7 startup sends 8101SY first, not edit VC.
        menu.addItem (10, "Test request: Edit buffer  (0040VC mt=7F — legacy)");
        menu.addItem (11, "Test request: A1 internal (8101VC mm=00)");
        menu.addItem (12, "Test request: B1 internal (8101VC mm=10)");
        menu.addSeparator();
        menu.addItem (30, "Auto-sync 64 internal voices (SYM7-style)");
        menu.addItem (31, "Auto-sync library — Voice+Preset+Multi (~2 min, SYM7)");
        menu.addItem (32, "Auto-sync extended — full SYM7 (+ SA/WV/PN/MT, ~5 min)");
        menu.addSeparator();
        menu.addItem (20, "Manual — wait for SY99 DUMP OUT");

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&btRequestVoice),
                            [this] (int result)
        {
            if (result == 10)
                beginBulkCaptureWithDumpRequest ("edit", 0x7F, 0x00);
            else if (result == 11)
                beginBulkCaptureWithSym78101Request ("a1", "8101VC", 0x00);
            else if (result == 12)
                beginBulkCaptureWithSym78101Request ("b1", "8101VC", 0x10);
            else if (result == 30)
                beginAutoSync64InternalVoices();
            else if (result == 31)
                beginFullLibraryStartupSync (false);
            else if (result == 32)
                beginFullLibraryStartupSync (true);
            else if (result == 20)
                beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                                  Sy99BulkCaptureSessionMode::singleAutoClose);
        });
    }

    void showReceiveFromSy99Menu()
    {
        PopupMenu menu;
        menu.addItem (1, "64 Voice Bank  (SY99 menu 05:64 Voice)");
        menu.addItem (2, "Single Voice   (SY99 menu 07:1 Voice)");
        menu.addSeparator();
        menu.addItem (3, "All dumps — continuous (each dump -> .syx, STOP when done)");

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&btReceive),
                            [this] (int result)
        {
            if (result == 1)
                beginBulkCapture (Sy99BulkCaptureKind::voiceBank64,
                                  Sy99BulkCaptureSessionMode::singleAutoClose);
            else if (result == 2)
                beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                                  Sy99BulkCaptureSessionMode::singleAutoClose);
            else if (result == 3)
                beginBulkCapture (Sy99BulkCaptureKind::receiveAll,
                                  Sy99BulkCaptureSessionMode::continuous);
        });
    }
    void buttonClicked (Button* button) override
    {
        Logger::writeToLog("Librairie -> ButtonClicked");
        if(button == &btSend)
        {
            String  bankName =appDirPath.getFullPathName()+"/";
            if (bankSelected.isNotEmpty())
            {
                bankName = bankName + bankSelected;
                Logger::writeToLog(bankName);
                if (! sender.send (adresseOscSendBank, bankSelected)) // [5]
                    Logger::writeToLog ("OSC erreur");;
                labelInfoLine.setText ("Bank send -> " + bankSelected, dontSendNotification);
            }
            else
            {
                labelInfoLine.setText ("No bank selected", dontSendNotification);
            }
        }
        else if (button == &btReceive)
        {
            showReceiveFromSy99Menu();
        }
        else if (button == &btStop)
        {
            if (sy99FullLibrarySyncSession().active)
            {
                auto& sync = sy99FullLibrarySyncSession();
                saveFullLibrarySyncCurrentPhase();

                if (bankList.activateSy99LibrarySession())
                {
                    refreshAutoSyncVoiceLists();
                    sy99RestoreLibrarySlotFromPersistence();
                }

                const bool hadFiles = sync.savedFiles.size() > 0 || bulkSessionSaveCount > 0;
                endFullLibrarySyncSession (hadFiles);
                return;
            }

            if (sy99AutoSync64Session().active)
            {
                if (sy99AutoSync64Session().accumulator.size() > 0)
                {
                    const File saved = saveAutoSync64CombinedCapture (sy99AutoSync64Session().accumulator);

                    if (saved.existsAsFile())
                    {
                        if (bankList.activateSy99LibrarySession())
                        {
                            refreshAutoSyncVoiceLists();
                            sy99RestoreLibrarySlotFromPersistence();
                        }
                        else
                            bankList.selectBankFileAndReloadVoices (saved);
                    }
                }

                endAutoSync64Session (sy99AutoSync64Session().accumulator.size() > 0
                                      || bulkSessionSaveCount > 0);
                return;
            }

            const bool hadBuffered = arraySysex.size() > 0;

            if (hadBuffered)
                commitBufferedDumpToLibrary();
            else
                labelInfoLine.setText ("No SysEx captured — check MIDI In port + Bulk Protect OFF",
                                       dontSendNotification);

            endBulkCaptureSession (hadBuffered || bulkSessionSaveCount > 0);
        }
        else if (button == &btImport)
        {
            chooser = std::make_unique<FileChooser> (
                "Import SY99 SysEx file",
                File::getSpecialLocation (File::userDocumentsDirectory),
                "*.syx;*.SYX");
            chooser->launchAsync (FileBrowserComponent::openMode
                                  | FileBrowserComponent::canSelectFiles,
                                  [this] (const FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == File()) return;

                if (bankList.importSyxFileToCapturesAndActivate (file))
                {
                    labelInfoLine.setText (LibrarySyncUi::txt (u8"Импорт: ")
                                           + file.getFileName(),
                                           dontSendNotification);
                }
                else
                {
                    labelInfoLine.setText (LibrarySyncUi::txt (u8"Импорт не удался: ")
                                           + file.getFileName(),
                                           dontSendNotification);
                }
            });
        }
        else if (button == &btExport)
        {
            if (bankSelected.isEmpty())
            {
                labelInfoLine.setText ("No bank selected to export", dontSendNotification);
                return;
            }
            File src;

            if (sy99IsSy99LibrarySessionBankLabel (bankSelected))
                src = sy99LibrarySession().voiceInternal;
            else
                src = libraryBankFileFromName (bankSelected);
            if (! src.existsAsFile())
            {
                labelInfoLine.setText ("Source missing: " + bankSelected,
                                       dontSendNotification);
                return;
            }
            chooser = std::make_unique<FileChooser> (
                "Export bank file",
                File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (bankSelected),
                "*.syx");
            chooser->launchAsync (FileBrowserComponent::saveMode
                                  | FileBrowserComponent::canSelectFiles
                                  | FileBrowserComponent::warnAboutOverwriting,
                                  [this, src] (const FileChooser& fc)
            {
                auto dest = fc.getResult();
                if (dest == File()) return;
                if (src.copyFileTo (dest))
                    labelInfoLine.setText ("Exported -> " + dest.getFullPathName(),
                                           dontSendNotification);
                else
                    labelInfoLine.setText ("Export failed", dontSendNotification);
            });
        }
        else if (button == &btSendVoice)
        {
            if (bankSelected.isEmpty())
            {
                labelInfoLine.setText ("Select a bank file first", dontSendNotification);
                return;
            }

            if (bankSelectedVoiceIndex < 0)
            {
                labelInfoLine.setText ("Click a voice slot (A–D) to open it", dontSendNotification);
                return;
            }

            const String target = libraryVoiceSy99InternalTarget (bankSelectedVoiceIndex);
            const String voiceLabel = libraryVoiceSlotLabel (bankSelectedVoiceIndex);

            AlertWindow::showOkCancelBox (
                AlertWindow::WarningIcon,
                "Write voice to SY99 internal memory?",
                "This overwrites " + target + " on the synthesizer (\"" + voiceLabel + "\").\n\n"
                "Browsing / Prev / Next only opens the voice in the editor and does not send MIDI.\n\n"
                "Continue?",
                "Write to SY99",
                "Cancel",
                this,
                ModalCallbackFunction::create ([this, target, voiceLabel] (int result)
                {
                    if (result != 1)
                    {
                        labelInfoLine.setText ("Write cancelled — \"" + voiceLabel
                                               + "\" still open in editor only",
                                               dontSendNotification);
                        return;
                    }

                    if (! sender.send (OSCAddressPattern (adresseOscSendVoice),
                                       bankSelected, (int32) bankSelectedVoiceIndex))
                    {
                        labelInfoLine.setText ("Write to SY99 failed (OSC)", dontSendNotification);
                        return;
                    }

                    labelInfoLine.setText ("Written to SY99 " + target
                                           + " — select that voice on the synth to play",
                                           dontSendNotification);
                }));
        }
        else if (button == &btRequestVoice)
        {
            showDumpRequestVerifyMenu();
        }
        else if (button == &btVoice)
        {
            switchLibraryContentPage (Sy99LibraryContentPage::internalVoices);
        }
        else if (button == &btPreset1)
        {
            switchLibraryContentPage (Sy99LibraryContentPage::preset1Voices);
        }
        else if (button == &btPreset2)
        {
            switchLibraryContentPage (Sy99LibraryContentPage::preset2Voices);
        }
        else if (button == &btMulti)
        {
            if (libraryContentPage() == Sy99LibraryContentPage::multiInternal)
                switchLibraryContentPage (Sy99LibraryContentPage::multiPreset);
            else
                switchLibraryContentPage (Sy99LibraryContentPage::multiInternal);
        }
    }
    
private:
    //       LibrairiePage& owner;
    ToggleButton toggleButton;
    
    
    
    
    // FIN DES XML FONCTIONS
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setRadioGroupId(77);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }

    void highlightLibraryVoiceSlot (int globalIdx) noexcept
    {
        voicesListA.sourceListBox.deselectAllRows();
        voicesListB.sourceListBox.deselectAllRows();
        voicesListC.sourceListBox.deselectAllRows();
        voicesListD.sourceListBox.deselectAllRows();

        if (libraryPageIsMulti (libraryContentPage()))
        {
            if (globalIdx < 0 || globalIdx >= kSy99LibraryMultiSlotCount)
                return;

            voicesListA.sourceListBox.selectRow (globalIdx);
            return;
        }

        auto& slots = libraryActiveSlotNames();

        if (globalIdx < 0 || globalIdx >= slots.size())
            return;

        const int rowsPerBank = libraryPageRowsPerBank();
        const int bankIdx = globalIdx / rowsPerBank;
        const int row = globalIdx % rowsPerBank;

        switch (bankIdx)
        {
            case 0: voicesListA.sourceListBox.selectRow (row); break;
            case 1: voicesListB.sourceListBox.selectRow (row); break;
            case 2: voicesListC.sourceListBox.selectRow (row); break;
            case 3: voicesListD.sourceListBox.selectRow (row); break;
            default: break;
        }
    }
    Label labelInfoLine {"", "Ready"};
    Label labelAutoSyncBanner;
    Label labelAutoSyncDetail;
    ProgressBar autoSyncProgressBar;
    double autoSyncProgressValue = 0.0;
    TextButton btSend {TRANS("SEND BANK->")};
    TextButton btReceive {TRANS("FROM SY99 <-")};
    TextButton btStop {"STOP"};

    TextButton btImport     {TRANS("IMPORT")};
    TextButton btExport     {TRANS("EXPORT")};
    TextButton btSendVoice  { TRANS ("WRITE SY99") };
    TextButton btRequestVoice {TRANS("REQUEST VOICE")};

    std::unique_ptr<FileChooser> chooser;

    TextButton btVoice {"VOICE"};
    TextButton btPreset1 {"PRE1"};
    TextButton btPreset2 {"PRE2"};
    TextButton btMulti {"MULTI"};
    TextButton btSeq{"SEQ"};

    //    TableListBox table { {}, this };;
    //   TableListBox tableBank { {}, this };;
    BankATableModel voicesListA;
    BankBTableModel voicesListB;
    BankCTableModel voicesListC;
    BankDTableModel voicesListD;
    BankTableModel bankList;
    
    int rowSelected;
    
    
    int oldSize;
    
    int numRows;
    OSCSender sender;  // [2]

    Sy99BulkCaptureKind bulkCaptureKind { Sy99BulkCaptureKind::voiceBank64 };
    Sy99BulkCaptureSessionMode bulkSessionMode { Sy99BulkCaptureSessionMode::singleAutoClose };
    int bulkSessionSaveCount = 0;
    juce::uint32 syncLastPartialRxUiMs = 0;

    bool bulkCapturePendingDumpRequest = false;
    bool bulkCaptureSym78101Request = false;
    String bulkCaptureSym78101LmType;
    uint8 bulkCaptureSym78101Byte28 = 0;
    int bulkCaptureDumpRequestTailVariant = sy99DumpRequestTailMtMmChecksum;
    bool bulkCaptureDumpRequestRetried = false;
    String bulkCaptureDumpRequestCaseTag;
    uint8 bulkCaptureDumpRequestMt = 0;
    uint8 bulkCaptureDumpRequestMm = 0;
    
};
//==============================================================================

//==============================================================================
/**
 This class shows how to implement a TableListBoxModel to show in a TableListBox.
 */
//==============================================================================
