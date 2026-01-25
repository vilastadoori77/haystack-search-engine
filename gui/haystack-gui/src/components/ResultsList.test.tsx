import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import ResultsList from './ResultsList';

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

describe('ResultsList', () => {
  const mockResults: SearchResult[] = [
    { docId: 1, score: 0.95, snippet: 'First result snippet' },
    { docId: 2, score: 0.87, snippet: 'Second result snippet' },
    { docId: 3, score: 0.75, snippet: 'Third result snippet' },
  ];

  const defaultProps: ResultsListProps = {
    results: [],
    isLoading: false,
    error: null,
    query: '',
  };

  // TC-016: Renders loading state when isLoading=true
  it('TC-016: renders loading state when isLoading=true', () => {
    // Given: ResultsList component with isLoading=true
    // When: Component is rendered
    render(<ResultsList {...defaultProps} isLoading={true} />);

    // Then: Loading spinner and "Searching..." text are displayed
    expect(screen.getByText(/searching/i)).toBeInTheDocument();
    // Spinner might be rendered as a loading indicator or aria-busy
    const loadingElement = screen.getByRole('status', { hidden: true }) || screen.getByText(/searching/i);
    expect(loadingElement).toBeInTheDocument();
  });

  // TC-017: Renders error state when error is not null
  it('TC-017: renders error state when error is not null', () => {
    // Given: ResultsList component with error="Server unavailable"
    const errorMessage = 'Server unavailable';
    // When: Component is rendered
    render(<ResultsList {...defaultProps} error={errorMessage} />);

    // Then: Error message is displayed in red
    const errorElement = screen.getByText(errorMessage);
    expect(errorElement).toBeInTheDocument();
    expect(errorElement).toHaveClass(/red|error|text-red/i);
  });

  // TC-018: Renders empty state when no results and query exists
  it('TC-018: renders empty state when no results and query exists', () => {
    // Given: ResultsList component with empty results and query="test"
    // When: Component is rendered
    render(<ResultsList {...defaultProps} results={[]} query="test" />);

    // Then: Empty state message "No results found..." is displayed
    expect(screen.getByText(/no results found/i)).toBeInTheDocument();
  });

  // TC-019: Does not render empty state when query is empty
  it('TC-019: does not render empty state when query is empty', () => {
    // Given: ResultsList component with empty results and empty query
    // When: Component is rendered
    render(<ResultsList {...defaultProps} results={[]} query="" />);

    // Then: Empty state message is not displayed
    expect(screen.queryByText(/no results found/i)).not.toBeInTheDocument();
  });

  // TC-020: Renders results when results array has items
  it('TC-020: renders results when results array has items', () => {
    // Given: ResultsList component with results array
    // When: Component is rendered
    render(<ResultsList {...defaultProps} results={mockResults} query="test" />);

    // Then: ResultCard components are rendered for each result
    expect(screen.getByText(/first result snippet/i)).toBeInTheDocument();
    expect(screen.getByText(/second result snippet/i)).toBeInTheDocument();
    expect(screen.getByText(/third result snippet/i)).toBeInTheDocument();
  });

  // TC-021: Results list shows correct count
  it('TC-021: results list shows correct count', () => {
    // Given: ResultsList component with 3 results
    // When: Component is rendered
    render(<ResultsList {...defaultProps} results={mockResults} query="test" />);

    // Then: Count text displays "Search Results (3 found)"
    expect(screen.getByText(/search results.*3.*found/i)).toBeInTheDocument();
  });

  // TC-022: Loading state takes priority over results
  it('TC-022: loading state takes priority over results', () => {
    // Given: ResultsList component with isLoading=true and results
    // When: Component is rendered
    render(
      <ResultsList
        {...defaultProps}
        isLoading={true}
        results={mockResults}
        query="test"
      />
    );

    // Then: Loading state is displayed, results are not
    expect(screen.getByText(/searching/i)).toBeInTheDocument();
    expect(screen.queryByText(/first result snippet/i)).not.toBeInTheDocument();
  });

  // TC-023: Error state takes priority over results
  it('TC-023: error state takes priority over results', () => {
    // Given: ResultsList component with error and results
    const errorMessage = 'Network error';
    // When: Component is rendered
    render(
      <ResultsList
        {...defaultProps}
        error={errorMessage}
        results={mockResults}
        query="test"
      />
    );

    // Then: Error message is displayed, results are not
    expect(screen.getByText(errorMessage)).toBeInTheDocument();
    expect(screen.queryByText(/first result snippet/i)).not.toBeInTheDocument();
  });
});
