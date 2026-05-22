/** Совпадает с JUCE MidiMessage::getMidiNoteName(note, true, true, 3). */
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'] as const;

const MIDDLE_C_OCTAVE = 3;

function clampMidiNote(note: number): number {
  return Math.max(0, Math.min(127, Math.round(note)));
}

/** MIDI 0…127 → «C-2» … «G8» (middle C = C3). */
export function midiNoteToName(note: number): string {
  const n = clampMidiNote(note);
  const octave = Math.floor(n / 12) + (MIDDLE_C_OCTAVE - 5);
  return `${NOTE_NAMES[n % 12]}${octave}`;
}

/** «C3», «c#4» → 60 или null. */
export function parseMidiNoteName(text: string): number | null {
  const t = text.trim();
  if (!t) {
    return null;
  }

  for (let n = 0; n <= 127; n++) {
    if (midiNoteToName(n).toLowerCase() === t.toLowerCase()) {
      return n;
    }
  }

  const match = /^([a-g])([#b]?)(-?\d+)$/i.exec(t.replace(/\s/g, ''));
  if (!match) {
    return null;
  }

  const letter = match[1].toUpperCase();
  let accidental = match[2] ?? '';
  const octave = Number.parseInt(match[3], 10);
  if (Number.isNaN(octave)) {
    return null;
  }

  const baseIndex: Record<string, number> = {
    C: 0,
    D: 2,
    E: 4,
    F: 5,
    G: 7,
    A: 9,
    B: 11,
  };
  let semitone = baseIndex[letter];
  if (semitone === undefined) {
    return null;
  }

  if (accidental === '#') {
    semitone += 1;
  } else if (accidental.toLowerCase() === 'b') {
    semitone -= 1;
  }

  const note = (octave + 5 - MIDDLE_C_OCTAVE) * 12 + semitone;
  if (note < 0 || note > 127) {
    return null;
  }
  return note;
}

export const MIDI_NOTE_OPTIONS = Array.from({ length: 128 }, (_, note) => ({
  note,
  label: midiNoteToName(note),
}));

/** Параметры, у которых raw 0…127 — номер MIDI-ноты (как ENLL/ENLH в Sysex77). */
export function paramUsesMidiNoteRange(param: {
  id: string;
  tag: string;
  uiLabel: string;
  rawMin: number;
  rawMax: number;
  decode: string;
  encode: string;
}): boolean {
  if (param.id === 'ENLL' || param.id === 'ENLH') {
    return true;
  }

  if (param.rawMin < 0 || param.rawMax > 127) {
    return false;
  }

  if (param.decode !== 'identity' || param.encode !== 'identity') {
    return false;
  }

  return /note limit/i.test(param.tag) || /note limit/i.test(param.uiLabel);
}

export function canUseMidiNoteRangeEditor(rawMin: number, rawMax: number): boolean {
  return rawMin >= 0 && rawMax <= 127;
}
