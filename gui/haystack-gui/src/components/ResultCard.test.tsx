import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import ResultCard from './ResultCard';

interface SearchResult {
  rank: number;
  docId: number;
  score: number;
  snippet: string;
  file_name?: string;
  page_number?: number;
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

  // TC-029: Phase 2.5 - renders file name when metadata is present
  it('TC-029: renders file name when metadata is present', () => {
    // Given: ResultCard with Phase 2.5 file metadata
    const resultWithMeta: SearchResult = {
      ...mockResult,
      file_name: 'report.pdf',
      page_number: 3,
    };
    // When: Component is rendered
    render(<ResultCard result={resultWithMeta} rank={1} />);

    // Then: File name is displayed instead of "Document {docId}"
    expect(screen.getByText('report.pdf')).toBeInTheDocument();
    expect(screen.queryByText(/document\s*5/i)).not.toBeInTheDocument();
  });

  // TC-030: Phase 2.5 - renders page number badge when metadata is present
  it('TC-030: renders page number when metadata is present', () => {
    // Given: ResultCard with Phase 2.5 file metadata
    const resultWithMeta: SearchResult = {
      ...mockResult,
      file_name: 'report.pdf',
      page_number: 3,
    };
    // When: Component is rendered
    render(<ResultCard result={resultWithMeta} rank={1} />);

    // Then: Page number badge is displayed
    expect(screen.getByText(/page\s*3/i)).toBeInTheDocument();
    expect(screen.getByLabelText('Page 3')).toBeInTheDocument();
  });

  // TC-031: Phase 2.5 - falls back to document ID without metadata
  it('TC-031: falls back to document ID when metadata is absent', () => {
    // Given: ResultCard without file metadata (pre-2.5 index)
    // When: Component is rendered
    render(<ResultCard result={mockResult} rank={1} />);

    // Then: Falls back to "Document {docId}", no page badge
    expect(screen.getByText(/document\s*5/i)).toBeInTheDocument();
    expect(screen.queryByText(/page\s*\d+/i)).not.toBeInTheDocument();
  });

  // TC-032: Phase 2.5 - treats empty file_name as absent metadata
  it('TC-032: treats empty file_name as absent metadata', () => {
    // Given: Backend sends file_name: "" for docs without metadata
    const resultEmptyMeta: SearchResult = {
      ...mockResult,
      file_name: '',
      page_number: 0,
    };
    // When: Component is rendered
    render(<ResultCard result={resultEmptyMeta} rank={1} />);

    // Then: Falls back to "Document {docId}", no page badge
    expect(screen.getByText(/document\s*5/i)).toBeInTheDocument();
    expect(screen.queryByText(/page\s*\d+/i)).not.toBeInTheDocument();
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
