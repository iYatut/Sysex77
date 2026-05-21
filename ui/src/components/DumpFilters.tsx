type DumpFiltersProps = {
  groupQuery: string;
  sourceQuery: string;
  groups: string[];
  onGroupQueryChange: (value: string) => void;
  onSourceQueryChange: (value: string) => void;
};

export function DumpFilters({
  groupQuery,
  sourceQuery,
  groups,
  onGroupQueryChange,
  onSourceQueryChange,
}: DumpFiltersProps) {
  return (
    <div className="params-filters">
      <label className="filter-field">
        <span>Group</span>
        <select value={groupQuery} onChange={(event) => onGroupQueryChange(event.target.value)}>
          <option value="">Все группы</option>
          {groups.map((group) => (
            <option key={group} value={group}>
              {group}
            </option>
          ))}
        </select>
      </label>

      <label className="filter-field">
        <span>Source</span>
        <select value={sourceQuery} onChange={(event) => onSourceQueryChange(event.target.value)}>
          <option value="">Все источники</option>
          <option value="live">live</option>
          <option value="bulk">bulk (8101 / 0040)</option>
        </select>
      </label>
    </div>
  );
}
