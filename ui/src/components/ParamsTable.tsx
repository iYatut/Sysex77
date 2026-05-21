import { useNavigate } from 'react-router-dom';
import type { ParamMeta } from '../types/paramMeta';
import { formatRawRange } from '../types/paramMeta';

type ParamsTableProps = {
  params: ParamMeta[];
};

function BoolCell({ value }: { value: boolean }) {
  return <span className={value ? 'flag flag--on' : 'flag flag--off'}>{value ? '✓' : '—'}</span>;
}

export function ParamsTable({ params }: ParamsTableProps) {
  const navigate = useNavigate();

  if (params.length === 0) {
    return <p className="empty-state">Параметры не найдены.</p>;
  }

  return (
    <div className="table-wrap">
      <table className="params-table">
        <thead>
          <tr>
            <th>Id</th>
            <th>tag</th>
            <th>uiLabel</th>
            <th>group</th>
            <th>perElement</th>
            <th>диапазон</th>
            <th>confirmedLive</th>
            <th>confirmedBulk8101</th>
            <th>confirmedBulk0040</th>
            <th>packedConflict</th>
          </tr>
        </thead>
        <tbody>
          {params.map((param) => (
            <tr
              key={param.id}
              className="params-row"
              tabIndex={0}
              onClick={() => navigate(`/params/${encodeURIComponent(param.id)}`)}
              onKeyDown={(event) => {
                if (event.key === 'Enter' || event.key === ' ') {
                  event.preventDefault();
                  navigate(`/params/${encodeURIComponent(param.id)}`);
                }
              }}
            >
              <td className="mono">{param.id}</td>
              <td className="mono">{param.tag}</td>
              <td>{param.uiLabel}</td>
              <td className="mono">{param.group}</td>
              <td>{param.perElement ? 'да' : 'нет'}</td>
              <td className="mono">{formatRawRange(param.rawMin, param.rawMax)}</td>
              <td>
                <BoolCell value={param.confidence.confirmedLive} />
              </td>
              <td>
                <BoolCell value={param.confidence.confirmedBulk8101} />
              </td>
              <td>
                <BoolCell value={param.confidence.confirmedBulk0040} />
              </td>
              <td>
                <BoolCell value={param.confidence.packedConflict} />
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
