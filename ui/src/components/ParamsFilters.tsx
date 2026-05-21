type ParamsFiltersProps = {
  tagQuery: string;
  groupQuery: string;
  groups: string[];
  onTagQueryChange: (value: string) => void;
  onGroupQueryChange: (value: string) => void;
};

export function ParamsFilters({
  tagQuery,
  groupQuery,
  groups,
  onTagQueryChange,
  onGroupQueryChange,
}: ParamsFiltersProps) {
  return (
    <div className="params-filters">
      <label className="filter-field">
        <span>Tag</span>
        <input
          type="search"
          value={tagQuery}
          placeholder="Поиск по tag или uiLabel"
          onChange={(event) => onTagQueryChange(event.target.value)}
        />
      </label>

      <label className="filter-field">
        <span>Group</span>
        <select
          value={groupQuery}
          onChange={(event) => onGroupQueryChange(event.target.value)}
        >
          <option value="">Все группы</option>
          {groups.map((group) => (
            <option key={group} value={group}>
              {group}
            </option>
          ))}
        </select>
      </label>
    </div>
  );
}
