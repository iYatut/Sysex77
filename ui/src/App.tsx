import { Navigate, Route, Routes } from 'react-router-dom';
import { ApiOfflineBanner } from './components/ApiOfflineBanner';
import { AppNav } from './components/AppNav';
import { DumpPage } from './pages/DumpPage';
import { ParamDetailPage } from './pages/ParamDetailPage';
import { ParamsListPage } from './pages/ParamsListPage';
import { TemplateEditorPage } from './pages/TemplateEditorPage';
import { TemplatesListPage } from './pages/TemplatesListPage';

export default function App() {
  return (
    <div className="app-shell">
      <AppNav />
      <ApiOfflineBanner />
      <Routes>
        <Route path="/" element={<Navigate to="/params" replace />} />
        <Route path="/params" element={<ParamsListPage />} />
        <Route path="/params/:id" element={<ParamDetailPage />} />
        <Route path="/dump" element={<DumpPage />} />
        <Route path="/templates" element={<TemplatesListPage />} />
        <Route path="/templates/:id" element={<TemplateEditorPage />} />
        <Route path="*" element={<Navigate to="/params" replace />} />
      </Routes>
    </div>
  );
}
