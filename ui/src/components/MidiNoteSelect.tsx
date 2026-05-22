import { MIDI_NOTE_OPTIONS, midiNoteToName } from '../utils/midiNotes';

type MidiNoteSelectProps = {
  value: number;
  onChange: (note: number) => void;
  className?: string;
  title?: string;
};

/** Компактный выбор ноты для ячеек таблицы (0…127). */
export function MidiNoteSelect({ value, onChange, className, title }: MidiNoteSelectProps) {
  const note = Math.max(0, Math.min(127, Math.round(value)));

  return (
    <select
      className={className ?? 'cell-input mono cell-input--midi-note'}
      value={note}
      title={title ?? midiNoteToName(note)}
      onChange={(event) => onChange(Number(event.target.value))}
    >
      {MIDI_NOTE_OPTIONS.map((opt) => (
        <option key={opt.note} value={opt.note}>
          {opt.label}
        </option>
      ))}
    </select>
  );
}
