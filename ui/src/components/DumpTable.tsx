import { useNavigate } from 'react-router-dom';
import { formatDumpValue, paramSourceTag } from '../types/paramDump';
import type { DumpTableRow } from '../utils/dumpRows';

type DumpTableProps = {
  rows: DumpTableRow[];
};

export function DumpTable({ rows }: DumpTableProps) {
  const navigate = useNavigate();

  if (rows.length === 0) {
    return <p className="empty-state">Строки dump не найдены.</p>;
  }

  return (
    <div className="table-wrap">
      <table className="params-table">
        <thead>
          <tr>
            <th>Id</th>
            <th>elementIndex</th>
            <th>tag</th>
            <th>uiLabel</th>
            <th>raw</th>
            <th>ui</th>
            <th>source</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((row) => (
            <tr
              key={`${row.id}:${row.elementIndex}`}
              className="params-row"
              tabIndex={0}
              onClick={() => navigate(`/params/${encodeURIComponent(row.id)}`)}
              onKeyDown={(event) => {
                if (event.key === 'Enter' || event.key === ' ') {
                  event.preventDefault();
                  navigate(`/params/${encodeURIComponent(row.id)}`);
                }
              }}
            >
              <td className="mono">{row.id}</td>
              <td className="mono">{row.elementIndex}</td>
              <td className="mono">{row.tag}</td>
              <td>{row.uiLabel}</td>
              <td className="mono">{formatDumpValue(row.raw)}</td>
              <td className="mono">{formatDumpValue(row.ui)}</td>
              <td className="mono">{paramSourceTag(row.source)}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
