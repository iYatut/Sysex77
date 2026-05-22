import { MIDI_NOTE_OPTIONS, midiNoteToName } from '../utils/midiNotes';

type MidiNoteRangeFieldProps = {
  label: string;
  value: number;
  onChange: (note: number) => void;
  min?: number;
  max?: number;
};

export function MidiNoteRangeField({
  label,
  value,
  onChange,
  min = 0,
  max = 127,
}: MidiNoteRangeFieldProps) {
  const clamped = Math.max(min, Math.min(max, Math.round(value)));
  const options = MIDI_NOTE_OPTIONS.filter((o) => o.note >= min && o.note <= max);

  return (
    <label className="form-field form-field--midi-note">
      <span>{label}</span>
      <div className="midi-note-field">
        <select
          className="mono"
          value={clamped}
          onChange={(event) => onChange(Number(event.target.value))}
        >
          {options.map((opt) => (
            <option key={opt.note} value={opt.note}>
              {opt.label} ({opt.note})
            </option>
          ))}
        </select>
        <span className="midi-note-field__hint mono" title="MIDI note number">
          {midiNoteToName(clamped)}
        </span>
      </div>
    </label>
  );
}
