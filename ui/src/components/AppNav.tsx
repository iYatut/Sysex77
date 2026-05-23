import { NavLink, useLocation } from 'react-router-dom';
import { focusLibraryVoiceFilters, isLibraryVoiceDetailPath } from '../utils/libraryVoiceFilters';

export function AppNav() {
  const location = useLocation();
  const showLibraryFilters = isLibraryVoiceDetailPath(location.pathname);

  return (
    <nav className="app-nav">
      <NavLink to="/params" className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}>
        Params
      </NavLink>
      <NavLink to="/dump" className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}>
        Dump
      </NavLink>
      <NavLink
        to="/library"
        className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}
      >
        Library
      </NavLink>
      <NavLink
        to="/templates"
        className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}
      >
        Templates
      </NavLink>
      {showLibraryFilters ? (
        <>
          <span className="app-nav__spacer" aria-hidden />
          <button type="button" className="app-nav__link" onClick={() => focusLibraryVoiceFilters()}>
            Фильтры
          </button>
        </>
      ) : null}
    </nav>
  );
}
