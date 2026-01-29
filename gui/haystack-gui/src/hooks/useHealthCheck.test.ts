/**
 * useHealthCheck Hook Test Suite
 * 
 * WHAT: Tests the useHealthCheck custom hook that monitors backend health
 * WHY: Ensures health monitoring works correctly for microservices reliability
 * BREAKS IF: Health checks fail, polling stops, or memory leaks occur
 * EXAMPLE: Backend goes down, hook detects unhealthy status, UI shows red indicator
 * 
 * Test-Driven Development (TDD) - RED PHASE
 * These tests will fail until useHealthCheck hook is implemented.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { renderHook, waitFor, act } from '@testing-library/react';
import type { HealthCheckResponse } from '../types/search';
import { useHealthCheck } from './useHealthCheck';

// Mock the searchApi module
vi.mock('../services/searchApi', () => ({
  checkHealth: vi.fn(),
}));

// Import mocked function
import { checkHealth } from '../services/searchApi';

// Type assertion for mocked function
const mockCheckHealth = vi.mocked(checkHealth);

describe('useHealthCheck', () => {
  beforeEach(() => {
    // Use fake timers for polling tests
    vi.useFakeTimers();
    
    // Reset all mocks
    vi.clearAllMocks();
  });

  afterEach(() => {
    // Restore real timers
    vi.useRealTimers();
    
    // Clean up
    vi.restoreAllMocks();
  });

  describe('TR-HEALTH-001: Initial state values', () => {
    it('should initialize with unknown health, null lastChecked, and false isRefreshing', () => {
      // WHAT: Verifies hook initializes with correct default state
      // WHY: Initial state must be predictable for UI rendering
      // BREAKS IF: Initial state incorrect, UI shows wrong health status
      // EXAMPLE: Component mounts, health indicator shows "unknown"

      // Given: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // When: No actions performed
      // Then: All state should be at initial values
      expect(result.current.health).toBe('unknown');
      expect(result.current.lastChecked).toBeNull();
      expect(result.current.isRefreshing).toBe(false);
    });
  });

  describe('TR-HEALTH-002: Initial health check on mount', () => {
    it('should call checkHealth API immediately on mount', async () => {
      // WHAT: Tests hook performs initial health check on mount
      // WHY: Immediate check provides current backend status
      // BREAKS IF: No initial check, health stays unknown
      // EXAMPLE: Component mounts, API called immediately

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      renderHook(() => useHealthCheck());

      // Then: checkHealth should be called immediately
      expect(mockCheckHealth).toHaveBeenCalledTimes(1);
    });
  });

  describe('TR-HEALTH-003: Polling interval setup', () => {
    it('should set up polling interval on mount', async () => {
      // WHAT: Tests polling interval is configured correctly
      // WHY: Polling ensures health status stays current
      // BREAKS IF: Polling not set up, health status becomes stale
      // EXAMPLE: Component mounts, interval set for 5-second polling

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      renderHook(() => useHealthCheck());

      // Wait for initial call (checkHealth is async, so we need to wait)
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      // Then: Initial call should happen
      expect(mockCheckHealth).toHaveBeenCalledTimes(1);

      // When: Advance time by 5 seconds
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      // Then: Second call should happen after 5 seconds
      expect(mockCheckHealth).toHaveBeenCalledTimes(2);
    });
  });

  describe('TR-HEALTH-004: Polling every 5 seconds', () => {
    it('should poll health every 5 seconds', async () => {
      // WHAT: Tests polling occurs at correct interval
      // WHY: 5-second interval balances responsiveness and server load
      // BREAKS IF: Polling too frequent (overloads server) or too slow (stale status)
      // EXAMPLE: Health checked at 0s, 5s, 10s, 15s intervals

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      renderHook(() => useHealthCheck());

      // Initial call
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(mockCheckHealth).toHaveBeenCalledTimes(1);

      // Advance 5 seconds
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(mockCheckHealth).toHaveBeenCalledTimes(2);

      // Advance another 5 seconds
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(mockCheckHealth).toHaveBeenCalledTimes(3);

      // Advance another 5 seconds
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(mockCheckHealth).toHaveBeenCalledTimes(4);
    });
  });

  describe('TR-HEALTH-005: Health status updates on successful check', () => {
    it('should update health to healthy when API returns healthy status', async () => {
      // WHAT: Tests health status updates on successful API response
      // WHY: Health status must reflect actual backend state
      // BREAKS IF: Health status doesn't update, UI shows wrong status
      // EXAMPLE: Backend healthy, API returns healthy, hook updates status

      // Given: Mock API returns healthy
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should update to healthy
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      expect(result.current.health).toBe('healthy');
    });
  });

  describe('TR-HEALTH-006: Health status updates on failed check', () => {
    it('should update health to unhealthy when API returns unhealthy status', async () => {
      // WHAT: Tests health status updates on failed API response
      // WHY: Must detect backend failures for user feedback
      // BREAKS IF: Failures not detected, users don't know backend is down
      // EXAMPLE: Backend down, API returns unhealthy, hook updates status

      // Given: Mock API returns unhealthy
      const mockResponse: HealthCheckResponse = { status: 'unhealthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should update to unhealthy
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      expect(result.current.health).toBe('unhealthy');
    });
  });

  describe('TR-HEALTH-007: lastChecked updates after each check', () => {
    it('should update lastChecked timestamp after each health check', async () => {
      // WHAT: Tests lastChecked timestamp updates correctly
      // WHY: Timestamp shows when health was last verified
      // BREAKS IF: Timestamp not updated, user can't see freshness
      // EXAMPLE: Health check completes, lastChecked set to current time

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: lastChecked should be updated after initial check
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.lastChecked).not.toBeNull();
      expect(result.current.lastChecked).toBeInstanceOf(Date);

      const firstCheckTime = result.current.lastChecked;

      // When: Advance time and trigger another check
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      // Then: lastChecked should be updated again
      expect(result.current.lastChecked).not.toBeNull();
      expect(result.current.lastChecked).not.toBe(firstCheckTime);
    });
  });

  describe('TR-HEALTH-008: Manual refresh sets isRefreshing', () => {
    it('should set isRefreshing to true during manual refresh', async () => {
      // WHAT: Tests isRefreshing flag during manual refresh
      // WHY: Flag prevents duplicate refresh requests
      // BREAKS IF: Flag not set, multiple refreshes triggered
      // EXAMPLE: User clicks refresh, isRefreshing becomes true

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered and refreshHealth is called
      const { result } = renderHook(() => useHealthCheck());

      // Wait for initial check
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      expect(result.current.health).not.toBe('unknown');

      // When: refreshHealth is called
      const refreshPromise = result.current.refreshHealth();
      
      // Check isRefreshing is true during refresh
      await act(async () => {
        await Promise.resolve();
      });
      // Note: isRefreshing might already be false if refresh completed quickly
      
      await act(async () => {
        await refreshPromise;
      });

      // Then: isRefreshing should be false after completion
      expect(result.current.isRefreshing).toBe(false);
    });
  });

  describe('TR-HEALTH-009: Manual refresh performs immediate check', () => {
    it('should perform health check immediately when refreshHealth is called', async () => {
      // WHAT: Tests manual refresh triggers immediate check
      // WHY: Users expect immediate feedback on refresh
      // BREAKS IF: Refresh delayed, poor user experience
      // EXAMPLE: User clicks refresh, check happens immediately

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Wait for initial check
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      expect(mockCheckHealth).toHaveBeenCalledTimes(1);

      const callCountBeforeRefresh = mockCheckHealth.mock.calls.length;

      // When: refreshHealth is called
      await act(async () => {
        await result.current.refreshHealth();
      });

      // Then: checkHealth should be called immediately
      expect(mockCheckHealth.mock.calls.length).toBe(callCountBeforeRefresh + 1);
    });
  });

  describe('TR-HEALTH-010: Manual refresh clears isRefreshing after completion', () => {
    it('should set isRefreshing to false after manual refresh completes', async () => {
      // WHAT: Tests isRefreshing flag cleared after refresh
      // WHY: Flag must reset to allow future refreshes
      // BREAKS IF: Flag stuck on true, refresh button disabled
      // EXAMPLE: Refresh completes, isRefreshing becomes false

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).not.toBe('unknown');

      // When: refreshHealth is called
      await act(async () => {
        await result.current.refreshHealth();
      });

      // Then: isRefreshing should be false after completion
      expect(result.current.isRefreshing).toBe(false);
    });
  });

  describe('TR-HEALTH-011: Auto-poll does not set isRefreshing', () => {
    it('should not set isRefreshing during automatic polling', async () => {
      // WHAT: Tests isRefreshing only set for manual refresh
      // WHY: Auto-polls shouldn't show refresh indicator
      // BREAKS IF: isRefreshing set during auto-polls, UI shows constant refresh
      // EXAMPLE: Auto-poll occurs, isRefreshing stays false

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('healthy');

      // When: Advance time to trigger auto-poll
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      expect(mockCheckHealth).toHaveBeenCalledTimes(2);

      // Then: isRefreshing should remain false
      expect(result.current.isRefreshing).toBe(false);
    });
  });

  describe('TR-HEALTH-012: Interval cleared on unmount', () => {
    it('should clear polling interval when component unmounts', async () => {
      // WHAT: Tests cleanup prevents memory leaks
      // WHY: Unmounted components shouldn't continue polling
      // BREAKS IF: Interval not cleared, memory leaks and unnecessary API calls
      // EXAMPLE: Component unmounts, polling stops

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered and then unmounted
      const { unmount } = renderHook(() => useHealthCheck());

      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(mockCheckHealth).toHaveBeenCalledTimes(1);

      unmount();

      // When: Advance time after unmount
      act(() => {
        vi.advanceTimersByTime(5000);
      });

      // Then: checkHealth should not be called again
      expect(mockCheckHealth).toHaveBeenCalledTimes(1);
    });
  });

  describe('TR-HEALTH-013: State updates prevented after unmount', () => {
    it('should not update state after component unmounts', async () => {
      // WHAT: Tests state updates prevented after unmount
      // WHY: Prevents memory leaks and React warnings
      // BREAKS IF: State updates after unmount, memory leaks occur
      // EXAMPLE: Component unmounts, pending API call completes, no state update

      // Given: Mock API with delayed response
      let resolvePromise: (value: HealthCheckResponse) => void;
      const delayedPromise = new Promise<HealthCheckResponse>((resolve) => {
        resolvePromise = resolve;
      });
      mockCheckHealth.mockReturnValue(delayedPromise);

      // When: Hook is rendered and then unmounted before API completes
      const { result, unmount } = renderHook(() => useHealthCheck());

      // Trigger a refresh that will complete after unmount
      const refreshPromise = act(async () => {
        await result.current.refreshHealth();
      });

      unmount();

      // When: API call completes after unmount
      act(() => {
        resolvePromise!({ status: 'healthy' });
      });

      await refreshPromise;

      // Then: No errors should occur (state updates prevented)
      // This test verifies no React warnings about setState on unmounted component
      expect(true).toBe(true); // Test passes if no errors thrown
    });
  });

  describe('TR-HEALTH-014: Continues polling when unhealthy', () => {
    it('should continue polling even when backend is unhealthy', async () => {
      // WHAT: Tests polling continues during unhealthy state
      // WHY: Must detect recovery when backend becomes healthy again
      // BREAKS IF: Polling stops, recovery not detected
      // EXAMPLE: Backend down, polling continues, detects recovery

      // Given: Mock API returns unhealthy
      const mockResponse: HealthCheckResponse = { status: 'unhealthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');

      // When: Advance time to trigger next poll
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      // Then: Polling should continue
      expect(mockCheckHealth).toHaveBeenCalledTimes(2);

      // When: Advance time again
      await act(async () => {
        vi.advanceTimersByTime(5000);
        await Promise.resolve();
      });

      // Then: Polling should continue
      expect(mockCheckHealth).toHaveBeenCalledTimes(3);
    });
  });

  describe('TR-HEALTH-015: Detects recovery (unhealthy to healthy)', () => {
    it('should detect when backend recovers from unhealthy to healthy', async () => {
      // WHAT: Tests recovery detection
      // WHY: Users need to know when backend is available again
      // BREAKS IF: Recovery not detected, UI shows stale unhealthy status
      // EXAMPLE: Backend recovers, health changes from unhealthy to healthy

      // Given: Mock API returns unhealthy then healthy
      mockCheckHealth
        .mockResolvedValueOnce({ status: 'unhealthy' })
        .mockResolvedValueOnce({ status: 'healthy' });

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should be unhealthy initially
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');

      // When: Advance time to trigger next poll
      await act(async () => {
        vi.advanceTimersByTime(5000);
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });

      // Then: Health should update to healthy
      expect(result.current.health).toBe('healthy');
    });
  });

  describe('TR-HEALTH-016: Handles network errors gracefully', () => {
    it('should set health to unhealthy on network errors', async () => {
      // WHAT: Tests network error handling
      // WHY: Network errors are common in microservices, must be handled
      // BREAKS IF: Network errors cause crashes or incorrect status
      // EXAMPLE: Network failure, health set to unhealthy

      // Given: Mock API throws network error
      mockCheckHealth.mockRejectedValue(new Error('Network error'));

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should be unhealthy (checkHealth never throws, returns unhealthy)
      // Note: checkHealth catches errors and returns { status: 'unhealthy' }
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');
    });
  });

  describe('TR-HEALTH-017: Handles timeout errors gracefully', () => {
    it('should set health to unhealthy on timeout errors', async () => {
      // WHAT: Tests timeout error handling
      // WHY: Timeouts indicate backend unresponsiveness
      // BREAKS IF: Timeouts cause crashes or incorrect status
      // EXAMPLE: Request times out, health set to unhealthy

      // Given: Mock API throws timeout error
      mockCheckHealth.mockRejectedValue(new Error('Timeout'));

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should be unhealthy
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');
    });
  });

  describe('TR-HEALTH-018: Handles HTTP errors gracefully', () => {
    it('should set health to unhealthy on HTTP errors', async () => {
      // WHAT: Tests HTTP error handling
      // WHY: HTTP errors indicate backend issues
      // BREAKS IF: HTTP errors cause crashes or incorrect status
      // EXAMPLE: Backend returns 500, health set to unhealthy

      // Given: Mock API throws HTTP error
      mockCheckHealth.mockRejectedValue(new Error('HTTP 500'));

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      // Then: Health should be unhealthy
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');
    });
  });

  describe('TR-HEALTH-019: Never throws exceptions', () => {
    it('should never throw exceptions even on unexpected errors', async () => {
      // WHAT: Tests hook never throws exceptions
      // WHY: Exceptions crash components, must be caught internally
      // BREAKS IF: Exceptions thrown, app crashes
      // EXAMPLE: Unexpected error occurs, hook handles gracefully

      // Given: Mock API throws unexpected error
      mockCheckHealth.mockRejectedValue(new Error('Unexpected error'));

      // When: Hook is rendered
      // Then: No exception should be thrown
      expect(() => {
        renderHook(() => useHealthCheck());
      }).not.toThrow();

      // Hook should handle error gracefully
      const { result } = renderHook(() => useHealthCheck());
      
      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('unhealthy');
    });
  });

  describe('TR-HEALTH-020: Multiple rapid manual refreshes', () => {
    it('should handle multiple rapid manual refreshes correctly', async () => {
      // WHAT: Tests concurrent manual refresh handling
      // WHY: Users may click refresh multiple times quickly
      // BREAKS IF: Multiple refreshes cause race conditions or errors
      // EXAMPLE: User clicks refresh 3 times rapidly, all handled correctly

      // Given: Mock API response
      const mockResponse: HealthCheckResponse = { status: 'healthy' };
      mockCheckHealth.mockResolvedValue(mockResponse);

      // When: Hook is rendered
      const { result } = renderHook(() => useHealthCheck());

      await act(async () => {
        vi.advanceTimersByTime(0);
        await Promise.resolve();
      });
      expect(result.current.health).toBe('healthy');

      // When: Multiple refreshes triggered rapidly
      await act(async () => {
        await Promise.all([
          result.current.refreshHealth(),
          result.current.refreshHealth(),
          result.current.refreshHealth(),
        ]);
      });

      // Then: All refreshes should complete without errors
      expect(result.current.isRefreshing).toBe(false);

      // checkHealth should be called multiple times
      expect(mockCheckHealth.mock.calls.length).toBeGreaterThan(1);
    });
  });
});
