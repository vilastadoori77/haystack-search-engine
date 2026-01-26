/**
 * ExampleQueries Component
 * 
 * Displays a set of pre-defined example query buttons that users can click
 * to quickly try different search queries. When clicked, automatically triggers
 * a search with the selected query.
 * 
 * Features:
 * - Renders 4 example query buttons with different query types:
 *   1. "migration" - Simple single-term query
 *   2. "migration validation" - Multi-term query (implicit AND)
 *   3. "migration OR validation" - Explicit OR query
 *   4. "schema -migration" - NOT query (exclusion)
 * - All buttons are disabled when isLoading is true
 * - Each button calls onQuerySelect callback with the query string when clicked
 * - Includes accessibility attributes (role="group", aria-labels on buttons)
 * 
 * @param onQuerySelect - Callback function called when a query button is clicked, receives the query string
 * @param isLoading - Boolean indicating if a search is currently in progress (disables all buttons)
 */
interface ExampleQueriesProps {
  onQuerySelect: (query: string) => void;
  isLoading: boolean;
}

const EXAMPLE_QUERIES = [
  'migration',
  'migration validation',
  'migration OR validation',
  'schema -migration',
];

export default function ExampleQueries({ onQuerySelect, isLoading }: ExampleQueriesProps) {
  return (
    <div
      role="group"
      aria-label="Example queries"
      className="mb-4"
    >
      <p className="text-sm text-gray-600 mb-2">Try these example queries:</p>
      <div className="flex flex-wrap gap-2">
        {EXAMPLE_QUERIES.map((query) => (
          <button
            key={query}
            onClick={() => onQuerySelect(query)}
            disabled={isLoading}
            aria-label={`Search for: ${query}`}
            className="px-3 py-1 text-sm bg-gray-100 hover:bg-gray-200 disabled:bg-gray-50 disabled:text-gray-400 disabled:cursor-not-allowed rounded border border-gray-300 transition-colors"
          >
            {query}
          </button>
        ))}
      </div>
    </div>
  );
}
