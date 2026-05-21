import { NavLink } from 'react-router-dom';

export function AppNav() {
  return (
    <nav className="app-nav">
      <NavLink to="/params" className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}>
        Params
      </NavLink>
      <NavLink to="/dump" className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}>
        Dump
      </NavLink>
      <NavLink
        to="/templates"
        className={({ isActive }) => (isActive ? 'app-nav__link is-active' : 'app-nav__link')}
      >
        Templates
      </NavLink>
    </nav>
  );
}
