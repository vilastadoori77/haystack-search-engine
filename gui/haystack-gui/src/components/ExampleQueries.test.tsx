import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import ExampleQueries from './ExampleQueries';

interface ExampleQueriesProps {
  onQuerySelect: (query: string) => void;
  isLoading: boolean;
}

describe('ExampleQueries', () => {
  let mockOnQuerySelect: ReturnType<typeof vi.fn>;

  beforeEach(() => {
    mockOnQuerySelect = vi.fn();
  });

  const defaultProps: ExampleQueriesProps = {
    onQuerySelect: mockOnQuerySelect,
    isLoading: false,
  };

  const exampleQueries = [
    'migration',
    'migration validation',
    'migration OR validation',
    'schema -migration',
  ];

  // TC-045: Renders all four example query buttons with correct labels
  it('TC-045: renders all four example query buttons with correct labels', () => {
    // Given: ExampleQueries component
    // When: Component is rendered
    render(<ExampleQueries {...defaultProps} />);

    // Then: All four buttons are displayed with correct labels
    exampleQueries.forEach((query) => {
      const button = screen.getByRole('button', { name: new RegExp(query, 'i') });
      expect(button).toBeInTheDocument();
    });
  });

  // TC-046: Calls onQuerySelect with correct query when button clicked
  it('TC-046: calls onQuerySelect with correct query when button clicked', async () => {
    // Given: ExampleQueries component
    const user = userEvent.setup();
    render(<ExampleQueries {...defaultProps} />);

    // When: First example query button is clicked
    const firstButton = screen.getByRole('button', { name: /migration$/i });
    await user.click(firstButton);

    // Then: onQuerySelect is called with "migration"
    expect(mockOnQuerySelect).toHaveBeenCalledTimes(1);
    expect(mockOnQuerySelect).toHaveBeenCalledWith('migration');
  });

  // TC-047: All buttons disabled when loading
  it('TC-047: all buttons disabled when loading', () => {
    // Given: ExampleQueries component with isLoading=true
    // When: Component is rendered
    render(<ExampleQueries {...defaultProps} isLoading={true} />);

    // Then: All buttons are disabled
    exampleQueries.forEach((query) => {
      const button = screen.getByRole('button', { name: new RegExp(query, 'i') });
      expect(button).toBeDisabled();
    });
  });

  // TC-048: All buttons enabled when not loading
  it('TC-048: all buttons enabled when not loading', () => {
    // Given: ExampleQueries component with isLoading=false
    // When: Component is rendered
    render(<ExampleQueries {...defaultProps} isLoading={false} />);

    // Then: All buttons are enabled
    exampleQueries.forEach((query) => {
      const button = screen.getByRole('button', { name: new RegExp(query, 'i') });
      expect(button).toBeEnabled();
    });
  });

  // Additional test: Each button calls onQuerySelect with correct query
  it('each button calls onQuerySelect with its respective query', async () => {
    // Given: ExampleQueries component
    const user = userEvent.setup();
    render(<ExampleQueries {...defaultProps} />);

    // When: Each button is clicked
    for (const query of exampleQueries) {
      const button = screen.getByRole('button', { name: new RegExp(query, 'i') });
      await user.click(button);
    }

    // Then: onQuerySelect is called with each query in order
    expect(mockOnQuerySelect).toHaveBeenCalledTimes(4);
    exampleQueries.forEach((query, index) => {
      expect(mockOnQuerySelect).toHaveBeenNthCalledWith(index + 1, query);
    });
  });

  // Additional test: Accessibility attributes
  it('has correct accessibility attributes', () => {
    // Given: ExampleQueries component
    // When: Component is rendered
    render(<ExampleQueries {...defaultProps} />);

    // Then: Container has role="group" and aria-label
    const container = screen.getByRole('group', { name: /example queries/i });
    expect(container).toBeInTheDocument();
    expect(container).toHaveAttribute('aria-label', 'Example queries');

    // And: Each button has aria-label
    exampleQueries.forEach((query) => {
      const button = screen.getByRole('button', { name: new RegExp(query, 'i') });
      expect(button).toHaveAttribute('aria-label', `Search for: ${query}`);
    });
  });
});
