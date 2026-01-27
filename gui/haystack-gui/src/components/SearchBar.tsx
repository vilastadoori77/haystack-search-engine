/**
 * SearchBar Component
 * 
 * Main search input component that provides the search interface.
 * Includes a text input, clear button, submit button, and limit slider.
 * 
 * Features:
 * - Search input field:
 *   - Placeholder: "Enter your search query..."
 *   - Disabled when isLoading is true
 *   - Calls onQueryChange on every keystroke
 *   - Triggers search on Enter key press (only if query is valid and not loading)
 * 
 * - Clear button:
 *   - Only visible when query has text (hidden when empty)
 *   - Calls onClear callback when clicked
 * 
 * - Submit button:
 *   - Text changes to "Searching..." when isLoading is true
 *   - Disabled when query is empty/invalid OR when isLoading is true
 *   - Calls onSearch callback when clicked
 * 
 * - Limit slider:
 *   - Range slider from 1 to 50
 *   - Displays current limit value: "Limit: {limit}"
 *   - Calls onLimitChange callback when value changes
 *   - Includes accessibility attributes (role="slider", aria-valuemin/max/now)
 * 
 * Validation:
 * - Uses validateQuery() utility to ensure query is non-empty and not whitespace-only
 * - Submit button and Enter key only trigger search if query is valid
 * 
 * @param query - Current search query string value
 * @param limit - Current limit value (1-50) for number of results
 * @param isLoading - Boolean indicating if a search is currently in progress
 * @param onQueryChange - Callback called when input value changes, receives new query string
 * @param onLimitChange - Callback called when slider value changes, receives new limit number
 * @param onSearch - Callback called when search should be executed (button click or Enter key)
 * @param onClear - Callback called when clear button is clicked
 */
import { validateQuery } from '../utils/validators';

interface SearchBarProps {
  query: string;
  limit: number;
  isLoading: boolean;
  onQueryChange: (query: string) => void;
  onLimitChange: (limit: number) => void;
  onSearch: () => void;
  onClear: () => void;
}

export default function SearchBar({
  query,
  limit,
  isLoading,
  onQueryChange,
  onLimitChange,
  onSearch,
  onClear,
}: SearchBarProps) {
  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    // TC-005: Calls onSearch when Enter key pressed (non-empty query)
    // TC-006: Does not call onSearch when Enter pressed with empty query
    if (e.key === 'Enter' && validateQuery(query) && !isLoading) {
      e.preventDefault();
      onSearch();
    }
  };

  // Also support onKeyPress for compatibility with testing libraries (deprecated but needed for tests)
  const handleKeyPress = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && validateQuery(query) && !isLoading) {
      e.preventDefault();
      onSearch();
    }
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    // TC-007: Calls onSearch when submit button clicked
    if (validateQuery(query) && !isLoading) {
      onSearch();
    }
  };

  return (
    <form onSubmit={handleSubmit} className="mb-6">
      <div className="flex gap-2 mb-4">
        {/* Search input */}
        <input
          type="text"
          value={query}
          onChange={(e) => onQueryChange(e.target.value)}
          onKeyDown={handleKeyDown}
          onKeyPress={handleKeyPress}
          placeholder="Enter your search query..."
          disabled={isLoading}
          className="flex-1 px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 disabled:bg-gray-100 disabled:cursor-not-allowed"
        />

        {/* Clear button - TC-009: visible when query has text, TC-010: hidden when empty */}
        {query.trim().length > 0 ? (
          <button
            type="button"
            onClick={onClear}
            aria-label="Clear query"
            className="px-4 py-2 bg-gray-200 hover:bg-gray-300 rounded-lg text-gray-700"
          >
            Clear
          </button>
        ) : null}

        {/* Submit button */}
        <button
          type="submit"
          disabled={!validateQuery(query) || isLoading}
          className="px-6 py-2 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-300 disabled:cursor-not-allowed text-white rounded-lg font-medium transition-colors"
        >
          {isLoading ? 'Searching...' : 'Search'}
        </button>
      </div>

      {/* Limit slider */}
      <div className="flex items-center gap-4">
        <label htmlFor="limit-slider" className="text-sm text-gray-700">
          Limit:
        </label>
        <input
          id="limit-slider"
          type="range"
          min="1"
          max="50"
          value={limit}
          onChange={(e) => onLimitChange(Number(e.target.value))}
          className="flex-1"
          role="slider"
          aria-valuemin={1}
          aria-valuemax={50}
          aria-valuenow={limit}
        />
        <span className="text-sm text-gray-700 min-w-[60px]">
          Limit: {limit}
        </span>
      </div>
    </form>
  );
}
