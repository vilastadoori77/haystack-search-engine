/**
 * App Component (Main Application Component)
 * 
 * Root component that orchestrates the entire Haystack Search Engine GUI.
 * Manages all application state and coordinates between child components.
 * 
 * State Management:
 * - Search State:
 *   - query: Current search query string
 *   - limit: Number of results to fetch (1-50)
 *   - results: Array of search results
 *   - isLoading: Boolean indicating if search is in progress
 *   - error: Error message string or null
 *   - metrics: Search metrics object (query, resultCount, latency) or null
 * 
 * - Health State:
 *   - health: Current API health status ('healthy', 'unhealthy', 'unknown')
 *   - healthLastChecked: Timestamp of last health check or null
 *   - isHealthRefreshing: Boolean indicating if health check is in progress
 * 
 * Features:
 * - Health Monitoring:
 *   - Performs initial health check on mount
 *   - Polls health endpoint every 5 seconds automatically
 *   - Provides manual refresh button via HealthStatus component
 * 
 * - Search Functionality:
 *   - Executes search via searchApi.search() when user submits query
 *   - Calculates and stores latency metrics
 *   - Handles errors gracefully and displays them in ResultsList
 *   - Updates metrics panel with query, result count, and latency
 * 
 * - Example Queries:
 *   - Auto-executes search when user clicks an example query button
 *   - Sets query value and immediately triggers search
 * 
 * - Clear Functionality:
 *   - Resets query, results, error, and metrics to initial state
 * 
 * Component Layout:
 * - Header: Title and HealthStatus component
 * - Main: SearchBar, ExampleQueries, MetricsPanel (conditional), ResultsList
 * 
 * API Integration:
 * - Uses searchApi.search() for search requests
 * - Uses searchApi.checkHealth() for health checks
 * - Handles all API errors and displays appropriate messages
 */
import { useCallback, useEffect, useRef } from 'react';
import SearchBar from './components/SearchBar';
import ResultsList from './components/ResultsList';
import MetricsPanel from './components/MetricsPanel';
import ExampleQueries from './components/ExampleQueries';
import HealthStatus from './components/HealthStatus';
import { useSearch, useHealthCheck } from './hooks';

// Removed old state management code - now using custom hooks
// Old code commented out:
// type HealthStatusType = 'healthy' | 'unhealthy' | 'unknown';
// interface SearchMetrics {
//   query: string;
//   resultCount: number;
//   latency: number;
// }

function App() {
  // Search state
  /*const [query, setQuery] = useState('');
  const [limit, setLimit] = useState(10);
  const [results, setResults] = useState<SearchResponse['results']>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [metrics, setMetrics] = useState<SearchMetrics | null>(null);

  // Health state
  const [health, setHealth] = useState<HealthStatusType>('unknown');
  const [healthLastChecked, setHealthLastChecked] = useState<Date | null>(null);
  const [isHealthRefreshing, setIsHealthRefreshing] = useState(false);*/

  // Use custom hooks for search and health check
  const {
    query,
    limit,
    results,
    metrics,
    error,
    isLoading,
    setQuery,
    executeSearch,
    setLimit,
    clearSearch, } = useSearch();

  const {
    health,
    lastChecked: healthLastChecked,
    isRefreshing: isHealthRefreshing,
    refreshHealth, } = useHealthCheck();

  // Track if query was set from example query selection (to auto-execute search)
  const exampleQueryRef = useRef<string | null>(null);

  // Auto-execute search when query changes from example query selection
  // This ensures React state update completes before executing search
  useEffect(() => {
    if (exampleQueryRef.current !== null && query === exampleQueryRef.current) {
      exampleQueryRef.current = null; // Reset flag
      executeSearch();
    }
  }, [query, executeSearch]);

  // Search is handled by the hook
  const handleSearch = executeSearch;

  //Handle example query selection
  const handleExampleQuerySelect = useCallback((selectedQuery: string) => {
    exampleQueryRef.current = selectedQuery; // Mark as example query
    setQuery(selectedQuery); // This triggers useEffect above
  }, [setQuery]);

  // Clear is handled by the hook
  const handleClear = clearSearch;
  // Health refresh is handled by the hook
  const handleHealthRefresh = refreshHealth;

  return (
    <div className="min-h-screen bg-gray-50">
      <div className="container mx-auto px-4 py-8 max-w-6xl">
        <header className="mb-8">
          <h1 className="text-3xl font-bold text-gray-900 mb-4">
            Haystack Search Engine
          </h1>
          <HealthStatus
            health={health}
            lastChecked={healthLastChecked}
            onRefresh={handleHealthRefresh}
            isRefreshing={isHealthRefreshing}
          />
        </header>

        <main>
          <SearchBar
            query={query}
            limit={limit}
            isLoading={isLoading}
            onQueryChange={setQuery}
            onLimitChange={setLimit}
            onSearch={handleSearch}
            onClear={handleClear}
          />

          <ExampleQueries
            onQuerySelect={handleExampleQuerySelect}
            isLoading={isLoading}
          />

          {metrics && <MetricsPanel metrics={metrics} />}

          <ResultsList
            results={results}
            isLoading={isLoading}
            error={error}
            query={query}
          />
        </main>
      </div>
    </div>
  );
}

export default App;
