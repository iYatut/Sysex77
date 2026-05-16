# Sysex77 — Development Journal
Target: Yamaha SY99 (SY77/TG77 planned)
Stack: JUCE 8.0.12 | VS2022 toolset v143 | Windows x64
Original author: nseaSeb (github.com/nseaSeb/Sysex77)

---

## DONE ✅

### v0.1.0 — Windows port + JUCE 8 compatibility (15.05.2026)
- Fixed Mac absolute paths in Sysex77.jucer
  (/Applications/JKnobMan133-mac/ → Ressources/)
- Removed Base64-encoded Mac path from defines attribute
- Created PNG stubs (placeholders):
  Ressources/SwitchFilter.png
  Ressources/FiltreControl.png
  Ressources/ButtonFilter.png
- Added Visual Studio 2022 exporter in Projucer
- Fixed all JUCE 8 API incompatibilities:
  * BinaryData: added OscAfm_png
  * Main.cpp: String(File)→getFullPathName(),
    removed SplashScreen, fixed setIconImage
  * MidiDemo.h: new MIDI device API
    (getAvailableDevices, identifier-based)
  * Controller.h / Config.h / AWMVue.h:
    unique_ptr for XmlDocument::parse
  * BankTableModel / VoicesTableModel:
    make_unique for setHeaderComponent
  * BankTableModel: AlertWindow callback variant
  * Config.h: CallOutBox unique_ptr + move
- First successful build: Sysex77.exe (7.3 MB, Release x64)

### v0.1.1 — Programmatic UI controls (15.05.2026)
- Source/FilterControls.h created (no PNG dependency):
  * FiltreControlLookAndFeel:
    rotary knob, dark gradient, orange arc (#FF8C00),
    glow pointer, soft shadow
  * ButtonFilterLookAndFeel:
    toggle button, 3-ring glow when active,
    dark #1a1a1a inactive state
  * SwitchFilter:
    3-state LP/BP/HP component,
    replaces MidiRadio, keeps getValue() interface,
    orange gradient active segment, pill container
- CommonFilter.h updated:
  * MidiRadio → SwitchFilter for radioFilter1Mode
  * FiltreControlLookAndFeel applied to:
    sliderFq1, sliderFq2, sliderResonnance,
    sliderLfoSens, sliderVelocity
  * ButtonFilterLookAndFeel applied to btFilter2
  * LF objects declared before sliders (correct destruction)
  * Destructor explicitly resets LF pointers
- FilterControls.h added to Sysex77.jucer Filter group

### v0.1.2 — ADSR envelope display for filters (15.05.2026)
- ADSR.h: added 6 helper methods to SADSR class:
  * clearSegments() — reset on element change
  * getSegmentCount()
  * getSegmentLevel(idx) / getSegmentRate(idx)
  * setSegmentLevel(idx,v) / setSegmentRate(idx,v)
- Filter1.h + Filter2.h (identical changes):
  * #include "ADSR.h" + ChangeListener inheritance
  * SADSR egFilter as first private member
  * Constructor: addAndMakeVisible + setModeHold(true)
  * changeListenerCallback(): segment dragged →
    updates all MidiSliders (dontSendNotification)
  * sliderValueChanged(): slider changed →
    setSegmentLevel/Rate + egFilter.repaint()
  * setElementNumber(): clearSegments() then
    6 addSegment() pairs:
    L0/R1, L1/R2, L2/R3, L3/R4, L4/RR1, RL1/RL2
    + setRelease() for RR2
  * resized(): egFilter.setBoundsRelative(0,0,0.65f,0.80f)
    sliders remain as right-side secondary input
- Style: orange gradient (#FF8C00), white hook points
- Build: success, exit code 0

### v0.1.4 — DeviceModel Architecture (15.05.2026)
- Source/DeviceModel.h — new singleton:
  * enum class YamahaDevice { SY77 = 1, TG77 = 2, SY99 = 3 }
  * getVoiceCount() — 128 for SY99, 64 for SY77/TG77
  * hasRAM() — true for all models
  * getWavesXML() — SY99Waves.xml or SY77Waves.xml
  * getSysExDeviceID() — delegates to sysexEngine byte
  * Listener interface with deviceChanged() callback
- Config.h — device selector dropdown connected:
  * comboBoxChanged → DeviceModel::getInstance().setDevice()
    mapping comboModel ID (1/2/3) directly to YamahaDevice
  * Constructor syncs comboModel from DeviceModel after loadParams()
    (dontSendNotification, handles first-run / invalid saved value)
- MidiDemo.h — replaced hardcoded 0x10 device-ID byte:
  * btBulk handler: 0x10 → getSysExDeviceID()
  * comboMod handler: 0x10 → getSysExDeviceID()
- DeviceModel.h registered in Sysex77.jucer Source group
- Build: 0 errors, 0 new warnings

---

## PLANNED 📋

### PRIORITY 1 — Architecture (before new features)
- [ ] SY99-only features toggle (DeviceModel model/count/XML done;
      UI and SysEx wiring still needed):
        - RAM waveforms (sample upload)
        - MIDI Data Recorder (MDR)
        - Waveform Card slot
        - Load correct Waves XML on device change
          (getWavesXML() exists; AWMVue not yet wired)

- [ ] Correct SysEx device ID byte per model:
      Done: MidiDemo btBulk + comboMod handlers
      TODO: audit all 0x10 literals in MidiSysex.h

- [ ] Voice Mode selector (modes 1-10):
      Mode 1:  1 AFM Poly
      Mode 2:  2 AFM Poly
      Mode 3:  4 AFM Mono  (extra memory)
      Mode 4:  1 AWM Poly
      Mode 5:  2 AWM Poly
      Mode 6:  1 AFM + 1 AWM
      Mode 7:  2 AFM + 1 AWM
      Mode 8:  4 AWM Poly  (extra memory)
      Mode 9:  1 AFM + 2 AWM
      Mode 10: 2 AFM + 2 AWM (extra memory)
      Extra memory flag for modes 3, 8, 10
      Dynamic UI: show/hide AFM/AWM element tabs
      AWM element: AFM waveform as source option
      SysEx: Voice Common address 01 00 00

### PRIORITY 2 — Core Missing SysEx
- [ ] Dump Request (F0 43 2n 34 ..):
      requestVoiceBankDump()    — all 64 voices
      requestSingleVoiceDump(n) — current voice
      requestMultiDump()
      requestEffectDump()
      requestSystemDump()
      Set up response listener + parse incoming data

- [ ] Effects Editor (SysEx 08..):
      Insertion Effect: type + parameters
      System Effect: Reverb / Chorus
      EQ parameters
      Effect Bulk send/receive (34 0D..)
      Two effect modulators per voice (MIDI CC/AT/vel)

### PRIORITY 3 — Standard MIDI (missing entirely)
- [ ] Program Change (patch switching)
- [ ] Bank Select (CC 0 / CC 32)
- [ ] Pitch Bend
- [ ] Channel Aftertouch
- [ ] CC 7 (Volume), CC 10 (Pan),
      CC 11 (Expression), CC 64 (Sustain)

### PRIORITY 4 — SY99 Exclusive
- [ ] PCM Sample Transfer:
      Waveform upload to RAM (SysEx 34 20-27..)
      Sample mapping (key range, loop type)
      Internal waveform set selection
- [ ] MIDI Data Recorder (MDR) support
- [ ] WaveBlade / Waveform Card integration

### PRIORITY 5 — UI Modernization
- [ ] Custom LookAndFeel for entire app
- [ ] Resizable window
- [ ] Animated envelope curves
- [ ] Dark theme polish (better fonts, spacing)

---

## KNOWN ISSUES 🐛
- PNG stubs in Ressources/ are placeholders
  (real graphics need JKnobMan export or keep programmatic)
- Projucer shows warning on save (non-blocking, safe to ignore)
- btMulti button disabled — Multi mode not implemented
- btSeq button disabled — Sequencer not implemented
- SysEx audit: 4 implemented / 19 partial / 25 missing
  out of 48 total SY99 commands

---

## AUDIT SUMMARY (15.05.2026)
Based on official Yamaha SY99 MIDI Data Format (SY99E2.PDF)
Total SysEx commands: 48
  Implemented:  4
  Partial:     19
  Missing:     25
Critical gaps: Dump Request, Effects, Multi mode,
               Sample Transfer, Program Change

---

## GIT STRATEGY
Branches:
  main     — stable builds only
  dev      — integration
  feature/ — one branch per feature

Tags: v0.1.0-windows, v0.2.0, etc.

## RELEASE HISTORY
  v0.1.4 — 15.05.2026 — DeviceModel singleton + Config wiring
  v0.1.2 — 15.05.2026 — ADSR filter envelopes
  v0.1.1 — 15.05.2026 — programmatic FilterControls
  v0.1.0 — 15.05.2026 — first Windows build (JUCE 8 port)
