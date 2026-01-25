import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import MetricsPanel from './MetricsPanel';

interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number; // milliseconds
}

interface MetricsPanelProps {
  metrics: SearchMetrics | null;
}

describe('MetricsPanel', () => {
  const mockMetrics: SearchMetrics = {
    query: 'migration',
    resultCount: 3,
    latency: 123,
  };

  // TC-039: Renders when metrics is not null
  it('TC-039: renders when metrics is not null', () => {
    // Given: MetricsPanel component with metrics object
    // When: Component is rendered
    render(<MetricsPanel metrics={mockMetrics} />);

    // Then: Panel is displayed
    const panel = screen.getByRole('region', { name: /search metrics/i });
    expect(panel).toBeInTheDocument();
  });

  // TC-040: Does not render when metrics is null
  it('TC-040: does not render when metrics is null', () => {
    // Given: MetricsPanel component with metrics=null
    // When: Component is rendered
    const { container } = render(<MetricsPanel metrics={null} />);

    // Then: Component returns null (nothing rendered)
    expect(container.firstChild).toBeNull();
  });

  // TC-041: Displays query text
  it('TC-041: displays query text', () => {
    // Given: MetricsPanel component with query="migration"
    // When: Component is rendered
    render(<MetricsPanel metrics={mockMetrics} />);

    // Then: Query text displays "Query: migration"
    expect(screen.getByText(/query:\s*migration/i)).toBeInTheDocument();
  });

  // TC-042: Displays result count
  it('TC-042: displays result count', () => {
    // Given: MetricsPanel component with resultCount=3
    // When: Component is rendered
    render(<MetricsPanel metrics={mockMetrics} />);

    // Then: Count text displays "3 result(s) found"
    expect(screen.getByText(/3\s*result.*found/i)).toBeInTheDocument();
  });

  // TC-043: Displays latency in milliseconds
  it('TC-043: displays latency in milliseconds', () => {
    // Given: MetricsPanel component with latency=123
    // When: Component is rendered
    render(<MetricsPanel metrics={mockMetrics} />);

    // Then: Latency text displays "Latency: 123ms"
    expect(screen.getByText(/latency:\s*123\s*ms/i)).toBeInTheDocument();
  });

  // TC-044: Handles plural/singular for result count
  it('TC-044: handles plural/singular for result count', () => {
    // Given: MetricsPanel component with resultCount=1
    const singleResultMetrics: SearchMetrics = {
      ...mockMetrics,
      resultCount: 1,
    };
    // When: Component is rendered
    render(<MetricsPanel metrics={singleResultMetrics} />);

    // Then: Count text displays "1 result found" (singular)
    expect(screen.getByText(/1\s*result\s*found/i)).toBeInTheDocument();
    expect(screen.queryByText(/results/i)).not.toBeInTheDocument();
  });

  // Additional test: Multiple results uses plural
  it('uses plural form for multiple results', () => {
    // Given: MetricsPanel component with resultCount=5
    const pluralMetrics: SearchMetrics = {
      ...mockMetrics,
      resultCount: 5,
    };
    // When: Component is rendered
    render(<MetricsPanel metrics={pluralMetrics} />);

    // Then: Count text displays "5 results found" (plural)
    expect(screen.getByText(/5\s*results?\s*found/i)).toBeInTheDocument();
  });

  // Additional test: Accessibility attributes
  it('has correct accessibility attributes', () => {
    // Given: MetricsPanel component
    // When: Component is rendered
    render(<MetricsPanel metrics={mockMetrics} />);

    // Then: Panel has role="region" and aria-label
    const panel = screen.getByRole('region', { name: /search metrics/i });
    expect(panel).toBeInTheDocument();
    expect(panel).toHaveAttribute('aria-label', 'Search metrics');
  });
});
