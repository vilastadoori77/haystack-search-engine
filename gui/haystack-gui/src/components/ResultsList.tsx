/**
 * ResultsList Component
 * 
 * Displays search results with proper state management and priority handling.
 * Shows different UI states based on loading, error, empty, or results conditions.
 * 
 * State Priority (in order):
 * 1. Loading state - Takes highest priority, shows spinner and "Searching..." text
 * 2. Error state - Shows error message in red when error is not null
 * 3. Empty state - Shows "No results found" message when results are empty AND query exists
 * 4. Results state - Displays list of ResultCard components when results are available
 * 
 * Features:
 * - Loading state: Shows animated spinner and "Searching..." text
 * - Error state: Displays error message in red text
 * - Empty state: Only shown when query exists but no results found
 * - Results state: Renders ResultCard for each result with rank (1-based)
 * - Shows result count in header: "Search Results ({count} found)"
 * - Includes accessibility attributes (role="status", aria-busy for loading)
 * 
 * @param results - Array of search result objects to display
 * @param isLoading - Boolean indicating if a search is currently in progress
 * @param error - Error message string to display, or null if no error
 * @param query - The search query string (used to determine if empty state should show)
 */
import ResultCard from './ResultCard';

interface SearchResult {
  docId: number;
  score: number;
  snippet: string;
}

interface ResultsListProps {
  results: SearchResult[];
  isLoading: boolean;
  error: string | null;
  query: string;
}

export default function ResultsList({
  results,
  isLoading,
  error,
  query,
}: ResultsListProps) {
  // TC-022: Loading state takes priority over results
  if (isLoading) {
    return (
      <div className="text-center py-8">
        <div
          role="status"
          aria-busy="true"
          className="inline-block animate-spin rounded-full h-8 w-8 border-b-2 border-blue-600 mb-2"
        />
        <p className="text-gray-600">Searching...</p>
      </div>
    );
  }

  // TC-023: Error state takes priority over results
  if (error !== null) {
    return (
      <div className="text-center py-8">
        <p className="text-red-600 font-medium">{error}</p>
      </div>
    );
  }

  // TC-018: Renders empty state when no results and query exists
  // TC-019: Does not render empty state when query is empty
  if (results.length === 0 && query.length > 0) {
    return (
      <div className="text-center py-8">
        <p className="text-gray-600">No results found for "{query}"</p>
      </div>
    );
  }

  // TC-020: Renders results when results array has items
  // TC-021: Results list shows correct count
  return (
    <div>
      <h2 className="text-lg font-semibold text-gray-800 mb-4">
        Search Results ({results.length} found)
      </h2>
      <div>
        {results.map((result, index) => (
          <ResultCard key={result.docId} result={result} rank={index + 1} />
        ))}
      </div>
    </div>
  );
}
