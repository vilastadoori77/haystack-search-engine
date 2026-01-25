import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import SearchBar from './SearchBar';

interface SearchBarProps {
  query: string;
  limit: number;
  isLoading: boolean;
  onQueryChange: (query: string) => void;
  onLimitChange: (limit: number) => void;
  onSearch: () => void;
  onClear: () => void;
}

describe('SearchBar', () => {
  let mockOnQueryChange: ReturnType<typeof vi.fn>;
  let mockOnLimitChange: ReturnType<typeof vi.fn>;
  let mockOnSearch: ReturnType<typeof vi.fn>;
  let mockOnClear: ReturnType<typeof vi.fn>;

  beforeEach(() => {
    mockOnQueryChange = vi.fn();
    mockOnLimitChange = vi.fn();
    mockOnSearch = vi.fn();
    mockOnClear = vi.fn();
  });

  const defaultProps: SearchBarProps = {
    query: '',
    limit: 10,
    isLoading: false,
    onQueryChange: mockOnQueryChange,
    onLimitChange: mockOnLimitChange,
    onSearch: mockOnSearch,
    onClear: mockOnClear,
  };

  // TC-001: Renders with empty input field
  it('TC-001: renders with empty input field', () => {
    // Given: SearchBar component with empty query
    // When: Component is rendered
    render(<SearchBar {...defaultProps} />);

    // Then: Input field is empty
    const input = screen.getByPlaceholderText('Enter your search query...');
    expect(input).toBeInTheDocument();
    expect(input).toHaveValue('');
  });

  // TC-002: Submit button disabled when query is empty
  it('TC-002: submit button disabled when query is empty', () => {
    // Given: SearchBar component with empty query
    // When: Component is rendered
    render(<SearchBar {...defaultProps} query="" />);

    // Then: Submit button is disabled
    const submitButton = screen.getByRole('button', { name: /search/i });
    expect(submitButton).toBeDisabled();
  });

  // TC-003: Submit button enabled when query has text
  it('TC-003: submit button enabled when query has text', () => {
    // Given: SearchBar component with non-empty query
    // When: Component is rendered
    render(<SearchBar {...defaultProps} query="test query" />);

    // Then: Submit button is enabled
    const submitButton = screen.getByRole('button', { name: /search/i });
    expect(submitButton).toBeEnabled();
  });

  // TC-004: Submit button disabled when loading
  it('TC-004: submit button disabled when loading', () => {
    // Given: SearchBar component with isLoading=true
    // When: Component is rendered
    render(<SearchBar {...defaultProps} query="test" isLoading={true} />);

    // Then: Submit button is disabled
    const submitButton = screen.getByRole('button', { name: /searching/i });
    expect(submitButton).toBeDisabled();
  });

  // TC-005: Calls onSearch when Enter key pressed (non-empty query)
  it('TC-005: calls onSearch when Enter key pressed (non-empty query)', async () => {
    // Given: SearchBar component with non-empty query
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} query="test" />);

    // When: Enter key is pressed in input field
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, '{Enter}');

    // Then: onSearch is called
    expect(mockOnSearch).toHaveBeenCalledTimes(1);
  });

  // TC-006: Does not call onSearch when Enter pressed with empty query
  it('TC-006: does not call onSearch when Enter pressed with empty query', async () => {
    // Given: SearchBar component with empty query
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} query="" />);

    // When: Enter key is pressed in input field
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, '{Enter}');

    // Then: onSearch is not called
    expect(mockOnSearch).not.toHaveBeenCalled();
  });

  // TC-007: Calls onSearch when submit button clicked
  it('TC-007: calls onSearch when submit button clicked', async () => {
    // Given: SearchBar component with non-empty query
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} query="test" />);

    // When: Submit button is clicked
    const submitButton = screen.getByRole('button', { name: /search/i });
    await user.click(submitButton);

    // Then: onSearch is called
    expect(mockOnSearch).toHaveBeenCalledTimes(1);
  });

  // TC-008: Calls onQueryChange when input changes
  it('TC-008: calls onQueryChange when input changes', async () => {
    // Given: SearchBar component
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} />);

    // When: User types in input field
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'test');

    // Then: onQueryChange is called with new value
    expect(mockOnQueryChange).toHaveBeenCalled();
    expect(mockOnQueryChange).toHaveBeenCalledWith('test');
  });

  // TC-009: Clear button visible when query has text
  it('TC-009: clear button visible when query has text', () => {
    // Given: SearchBar component with non-empty query
    // When: Component is rendered
    render(<SearchBar {...defaultProps} query="test" />);

    // Then: Clear button is visible
    const clearButton = screen.getByRole('button', { name: /clear/i });
    expect(clearButton).toBeInTheDocument();
  });

  // TC-010: Clear button hidden when query is empty
  it('TC-010: clear button hidden when query is empty', () => {
    // Given: SearchBar component with empty query
    // When: Component is rendered
    render(<SearchBar {...defaultProps} query="" />);

    // Then: Clear button is not visible
    const clearButton = screen.queryByRole('button', { name: /clear/i });
    expect(clearButton).not.toBeInTheDocument();
  });

  // TC-011: Calls onClear when clear button clicked
  it('TC-011: calls onClear when clear button clicked', async () => {
    // Given: SearchBar component with non-empty query
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} query="test" />);

    // When: Clear button is clicked
    const clearButton = screen.getByRole('button', { name: /clear/i });
    await user.click(clearButton);

    // Then: onClear is called
    expect(mockOnClear).toHaveBeenCalledTimes(1);
  });

  // TC-012: Slider updates limit value (calls onLimitChange)
  it('TC-012: slider updates limit value (calls onLimitChange)', async () => {
    // Given: SearchBar component with limit slider
    const user = userEvent.setup();
    render(<SearchBar {...defaultProps} limit={10} />);

    // When: User changes slider value
    const slider = screen.getByRole('slider');
    await user.type(slider, '25');

    // Then: onLimitChange is called with new value
    expect(mockOnLimitChange).toHaveBeenCalled();
  });

  // TC-013: Slider displays current limit text ("Limit: 15")
  it('TC-013: slider displays current limit text', () => {
    // Given: SearchBar component with limit=15
    // When: Component is rendered
    render(<SearchBar {...defaultProps} limit={15} />);

    // Then: Limit text displays "Limit: 15"
    expect(screen.getByText(/limit:\s*15/i)).toBeInTheDocument();
  });

  // TC-014: Slider respects min/max bounds (1-50)
  it('TC-014: slider respects min/max bounds (1-50)', () => {
    // Given: SearchBar component with limit slider
    // When: Component is rendered
    render(<SearchBar {...defaultProps} limit={25} />);

    // Then: Slider has min=1 and max=50
    const slider = screen.getByRole('slider');
    expect(slider).toHaveAttribute('min', '1');
    expect(slider).toHaveAttribute('max', '50');
  });

  // TC-015: Input disabled when loading
  it('TC-015: input disabled when loading', () => {
    // Given: SearchBar component with isLoading=true
    // When: Component is rendered
    render(<SearchBar {...defaultProps} isLoading={true} />);

    // Then: Input field is disabled
    const input = screen.getByPlaceholderText('Enter your search query...');
    expect(input).toBeDisabled();
  });
});
