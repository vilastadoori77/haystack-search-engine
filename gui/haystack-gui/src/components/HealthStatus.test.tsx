import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import HealthStatus from './HealthStatus';

type HealthStatusType = 'healthy' | 'unhealthy' | 'unknown';

interface HealthStatusProps {
  health: HealthStatusType;
  lastChecked: Date | null;
  onRefresh: () => void;
  isRefreshing: boolean;
}

describe('HealthStatus', () => {
  let mockOnRefresh: ReturnType<typeof vi.fn>;

  beforeEach(() => {
    mockOnRefresh = vi.fn();
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  const defaultProps: HealthStatusProps = {
    health: 'unknown',
    lastChecked: null,
    onRefresh: mockOnRefresh,
    isRefreshing: false,
  };

  // TC-029: Renders green dot when healthy
  it('TC-029: renders green dot when healthy', () => {
    // Given: HealthStatus component with health="healthy"
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} health="healthy" />);

    // Then: Green dot indicator is displayed
    const indicator = screen.getByRole('status', { hidden: true }) || screen.getByTestId('health-indicator');
    expect(indicator).toBeInTheDocument();
    expect(indicator).toHaveClass(/green|bg-green/i);
  });

  // TC-030: Renders red dot when unhealthy
  it('TC-030: renders red dot when unhealthy', () => {
    // Given: HealthStatus component with health="unhealthy"
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} health="unhealthy" />);

    // Then: Red dot indicator is displayed
    const indicator = screen.getByRole('status', { hidden: true }) || screen.getByTestId('health-indicator');
    expect(indicator).toBeInTheDocument();
    expect(indicator).toHaveClass(/red|bg-red/i);
  });

  // TC-031: Renders gray dot when unknown
  it('TC-031: renders gray dot when unknown', () => {
    // Given: HealthStatus component with health="unknown"
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} health="unknown" />);

    // Then: Gray dot indicator is displayed
    const indicator = screen.getByRole('status', { hidden: true }) || screen.getByTestId('health-indicator');
    expect(indicator).toBeInTheDocument();
    expect(indicator).toHaveClass(/gray|bg-gray/i);
  });

  // TC-032: Displays "Healthy" text when healthy
  it('TC-032: displays "Healthy" text when healthy', () => {
    // Given: HealthStatus component with health="healthy"
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} health="healthy" />);

    // Then: "Healthy" text is displayed
    expect(screen.getByText('Healthy')).toBeInTheDocument();
  });

  // TC-033: Displays "Unavailable" text when unhealthy
  it('TC-033: displays "Unavailable" text when unhealthy', () => {
    // Given: HealthStatus component with health="unhealthy"
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} health="unhealthy" />);

    // Then: "Unavailable" text is displayed
    expect(screen.getByText('Unavailable')).toBeInTheDocument();
  });

  // TC-034: Displays last checked timestamp
  it('TC-034: displays last checked timestamp', () => {
    // Given: HealthStatus component with lastChecked=2 seconds ago
    const twoSecondsAgo = new Date(Date.now() - 2000);
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} lastChecked={twoSecondsAgo} />);

    // Then: Timestamp displays "2s ago"
    expect(screen.getByText(/2s ago/i)).toBeInTheDocument();
  });

  // TC-035: Displays "Never" when lastChecked is null
  it('TC-035: displays "Never" when lastChecked is null', () => {
    // Given: HealthStatus component with lastChecked=null
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} lastChecked={null} />);

    // Then: "Never" text is displayed
    expect(screen.getByText('Never')).toBeInTheDocument();
  });

  // TC-036: Calls onRefresh when refresh button clicked
  it('TC-036: calls onRefresh when refresh button clicked', async () => {
    // Given: HealthStatus component with refresh button
    const user = userEvent.setup({ delay: null });
    render(<HealthStatus {...defaultProps} />);

    // When: Refresh button is clicked
    const refreshButton = screen.getByRole('button', { name: /refresh/i });
    await user.click(refreshButton);

    // Then: onRefresh is called
    expect(mockOnRefresh).toHaveBeenCalledTimes(1);
  });

  // TC-037: Refresh button disabled when refreshing
  it('TC-037: refresh button disabled when refreshing', () => {
    // Given: HealthStatus component with isRefreshing=true
    // When: Component is rendered
    render(<HealthStatus {...defaultProps} isRefreshing={true} />);

    // Then: Refresh button is disabled
    const refreshButton = screen.getByRole('button', { name: /refresh/i });
    expect(refreshButton).toBeDisabled();
  });

  // TC-038: Timestamp updates over time
  it('TC-038: timestamp updates over time', () => {
    // Given: HealthStatus component with lastChecked=5 seconds ago
    const fiveSecondsAgo = new Date(Date.now() - 5000);
    const { rerender } = render(<HealthStatus {...defaultProps} lastChecked={fiveSecondsAgo} />);

    // When: Component initially renders
    // Then: Timestamp displays "5s ago"
    expect(screen.getByText(/5s ago/i)).toBeInTheDocument();

    // When: Time advances by 2 seconds
    vi.advanceTimersByTime(2000);
    rerender(<HealthStatus {...defaultProps} lastChecked={fiveSecondsAgo} />);

    // Then: Timestamp updates to "7s ago"
    expect(screen.getByText(/7s ago/i)).toBeInTheDocument();
  });
});
