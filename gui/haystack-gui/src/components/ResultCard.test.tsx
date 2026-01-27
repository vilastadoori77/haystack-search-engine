import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import ResultCard from './ResultCard';

interface SearchResult {
  rank: number;
  docId: number;
  score: number;
  snippet: string;
}

describe('ResultCard', () => {
  const mockResult: SearchResult = {
    rank: 1,
    docId: 5,
    score: 2.456789,
    snippet: 'This is a test snippet that contains relevant information about the search query.',
  };

  // TC-024: Renders rank correctly
  it('TC-024: renders rank correctly', () => {
    // Given: ResultCard component with rank=1
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={1} />);

    // Then: Rank displays "#1"
    expect(screen.getByText('#1')).toBeInTheDocument();
  });

  // TC-025: Renders document ID correctly
  it('TC-025: renders document ID correctly', () => {
    // Given: ResultCard component with docId=5
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={1} />);

    // Then: Document ID displays "Document 5"
    expect(screen.getByText(/document\s*5/i)).toBeInTheDocument();
  });

  // TC-026: Renders score with 3 decimal places
  it('TC-026: renders score with 3 decimal places', () => {
    // Given: ResultCard component with score=2.456789
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={1} />);

    // Then: Score displays "2.457" (formatted to 3 decimals)
    expect(screen.getByText(/2\.457/i)).toBeInTheDocument();
  });

  // TC-027: Renders snippet text
  it('TC-027: renders snippet text', () => {
    // Given: ResultCard component with snippet text
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={1} />);

    // Then: Snippet text is displayed
    expect(screen.getByText(mockResult.snippet)).toBeInTheDocument();
  });

  // TC-028: Handles long snippets
  it('TC-028: handles long snippets', () => {
    // Given: ResultCard component with very long snippet
    const longSnippet = 'A'.repeat(500) + ' relevant content';
    const longResult: SearchResult = {
      ...mockResult,
      snippet: longSnippet,
    };
    // When: Component is rendered
    render(<ResultCard result={longResult} rank={1} />);

    // Then: Snippet is displayed (either truncated or full)
    const snippetElement = screen.getByText(/relevant content/i);
    expect(snippetElement).toBeInTheDocument();
  });

  // Additional test: Accessibility attributes
  it('has correct accessibility attributes', () => {
    // Given: ResultCard component
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={2} />);

    // Then: Card has role="article" and aria-label
    const card = screen.getByRole('article', { name: /search result 2/i });
    expect(card).toBeInTheDocument();
    expect(card).toHaveAttribute('aria-label', 'Search result 2');

    // And: Score has aria-label
    const scoreElement = screen.getByLabelText(/relevance score/i);
    expect(scoreElement).toBeInTheDocument();
  });
});
