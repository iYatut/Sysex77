import { Link } from 'react-router-dom';

type LibraryBreadcrumbsProps = {
  items: Array<{ label: string; to?: string }>;
};

export function LibraryBreadcrumbs({ items }: LibraryBreadcrumbsProps) {
  return (
    <nav className="library-breadcrumbs" aria-label="Library navigation">
      {items.map((item, index) => {
        const isLast = index === items.length - 1;

        return (
          <span key={`${item.label}-${index}`} className="library-breadcrumbs__item">
            {item.to && !isLast ? <Link to={item.to}>{item.label}</Link> : <span>{item.label}</span>}
            {!isLast && <span className="library-breadcrumbs__sep">/</span>}
          </span>
        );
      })}
    </nav>
  );
}
