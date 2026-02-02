/**
 * useSearch Hook Test Suite
 * 
 * WHAT: Tests the useSearch custom hook that manages search state and API calls
 * WHY: Ensures stateless search functionality works correctly in microservices architecture
 * BREAKS IF: Hook doesn't properly manage state, handle errors, or call APIs correctly
 * EXAMPLE: User searches for "migration", hook calls API, updates results and metrics
 * 
 * Test-Driven Development (TDD) - RED PHASE
 * These tests will fail until useSearch hook is implemented.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { renderHook, waitFor, act } from '@testing-library/react';
import type { SearchResult, SearchResponse } from '../types/search';
import { useSearch } from './useSearch';

// Mock the searchApi module
vi.mock('../services/searchApi', () => ({
  search: vi.fn(),
}));

// Mock the validators module
vi.mock('../utils/validators', () => ({
  validateQuery: vi.fn(),
  validateLimit: vi.fn(),
}));

// Import mocked functions
import { search } from '../services/searchApi';
import { validateQuery, validateLimit } from '../utils/validators';

// Type assertions for mocked functions
const mockSearch = vi.mocked(search);
const mockValidateQuery = vi.mocked(validateQuery);
const mockValidateLimit = vi.mocked(validateLimit);

describe('useSearch', () => {
  beforeEach(() => {
    // Reset all mocks before each test
    vi.clearAllMocks();
    
    // Default mock implementations
    mockValidateQuery.mockReturnValue(true);
    mockValidateLimit.mockReturnValue(true);
  });

  afterEach(() => {
    // Clean up after each test
    vi.restoreAllMocks();
  });

  describe('TR-SEARCH-001: Initial state values', () => {
    it('should initialize with empty query, default limit, empty results, null metrics, null error, and false loading', () => {
      // WHAT: Verifies hook initializes with correct default state
      // WHY: Ensures predictable initial state for stateless microservices
      // BREAKS IF: Initial state is incorrect, causing UI inconsistencies
      // EXAMPLE: Component mounts, expects empty search state

      // Given: Hook is rendered
      const { result } = renderHook(() => useSearch());

      // When: No actions performed
      // Then: All state should be at initial values
      expect(result.current.query).toBe('');
      expect(result.current.limit).toBe(10);
      expect(result.current.results).toEqual([]);
      expect(result.current.metrics).toBeNull();
      expect(result.current.error).toBeNull();
      expect(result.current.isLoading).toBe(false);
    });
  });

  describe('TR-SEARCH-002: setQuery updates query state', () => {
    it('should update query state when setQuery is called', () => {
      // WHAT: Tests setQuery function updates query state immediately
      // WHY: User input must be captured instantly for responsive UI
      // BREAKS IF: Query state doesn't update, user sees stale input
      // EXAMPLE: User types "migration", query updates to "migration"

      // Given: Hook is rendered with initial empty query
      const { result } = renderHook(() => useSearch());
      expect(result.current.query).toBe('');

      // When: setQuery is called with new value
      act(() => {
        result.current.setQuery('test query');
      });

      // Then: Query state should be updated
      expect(result.current.query).toBe('test query');
    });

    it('should preserve whitespace in query when setQuery is called', () => {
      // WHAT: Tests setQuery preserves whitespace (trimming happens in executeSearch)
      // WHY: Preserves exact user input until search execution
      // BREAKS IF: Whitespace trimmed prematurely, user loses input context
      // EXAMPLE: User types "  migration  ", query stores "  migration  "

      // Given: Hook is rendered
      const { result } = renderHook(() => useSearch());

      // When: setQuery is called with whitespace
      act(() => {
        result.current.setQuery('  test query  ');
      });

      // Then: Query should preserve whitespace
      expect(result.current.query).toBe('  test query  ');
    });
  });

  describe('TR-SEARCH-003: setLimit validates and updates limit', () => {
    it('should update limit when valid value is provided', () => {
      // WHAT: Tests setLimit validates and updates limit for valid values
      // WHY: Ensures limit stays within API constraints (1-50)
      // BREAKS IF: Invalid limits cause API errors or unexpected behavior
      // EXAMPLE: User sets limit to 25, hook updates limit to 25

      // Given: Hook is rendered with default limit of 10
      const { result } = renderHook(() => useSearch());
      expect(result.current.limit).toBe(10);
      mockValidateLimit.mockReturnValue(true);

      // When: setLimit is called with valid value
      act(() => {
        result.current.setLimit(25);
      });

      // Then: Limit should be updated and validateLimit should be called
      expect(mockValidateLimit).toHaveBeenCalledWith(25);
      expect(result.current.limit).toBe(25);
    });

    it('should handle boundary values (1 and 50)', () => {
      // WHAT: Tests setLimit accepts boundary values
      // WHY: Boundary values are valid and should work
      // BREAKS IF: Boundary values rejected, limiting user options
      // EXAMPLE: User sets limit to 1 or 50, both should work

      // Given: Hook is rendered
      const { result } = renderHook(() => useSearch());
      mockValidateLimit.mockReturnValue(true);

      // When: setLimit is called with boundary values
      act(() => {
        result.current.setLimit(1);
      });
      expect(result.current.limit).toBe(1);

      act(() => {
        result.current.setLimit(50);
      });

      // Then: Both boundary values should be accepted
      expect(result.current.limit).toBe(50);
    });
  });

  describe('TR-SEARCH-004: setLimit rejects invalid values', () => {
    it('should not update limit when value is less than 1', () => {
      // WHAT: Tests setLimit rejects values below minimum
      // WHY: Prevents API errors from invalid limit values
      // BREAKS IF: Invalid limits cause API failures
      // EXAMPLE: User tries to set limit to 0, limit stays unchanged

      // Given: Hook is rendered with limit of 10
      const { result } = renderHook(() => useSearch());
      const initialLimit = result.current.limit;
      mockValidateLimit.mockReturnValue(false);

      // When: setLimit is called with invalid value (< 1)
      act(() => {
        result.current.setLimit(0);
      });

      // Then: Limit should remain unchanged
      expect(mockValidateLimit).toHaveBeenCalledWith(0);
      expect(result.current.limit).toBe(initialLimit);
    });

    it('should not update limit when value is greater than 50', () => {
      // WHAT: Tests setLimit rejects values above maximum
      // WHY: Prevents API errors from exceeding backend limits
      // BREAKS IF: Large limits cause API failures or performance issues
      // EXAMPLE: User tries to set limit to 100, limit stays unchanged

      // Given: Hook is rendered with limit of 10
      const { result } = renderHook(() => useSearch());
      const initialLimit = result.current.limit;
      mockValidateLimit.mockReturnValue(false);

      // When: setLimit is called with invalid value (> 50)
      act(() => {
        result.current.setLimit(51);
      });

      // Then: Limit should remain unchanged
      expect(mockValidateLimit).toHaveBeenCalledWith(51);
      expect(result.current.limit).toBe(initialLimit);
    });
  });

  describe('TR-SEARCH-005: executeSearch calls API with current query and limit', () => {
    it('should call search API with trimmed query and current limit', async () => {
      // WHAT: Tests executeSearch calls API with correct parameters
      // WHY: Ensures API receives correct query and limit values
      // BREAKS IF: Wrong parameters sent, API returns incorrect results
      // EXAMPLE: Query "  migration  " with limit 25, API called with "migration" and 25

      // Given: Hook with query and limit set
      const { result } = renderHook(() => useSearch());
      const mockResponse: SearchResponse = {
        query: 'migration',
        results: [],
      };
      mockSearch.mockResolvedValue(mockResponse);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('  migration  ');
        result.current.setLimit(25);
      });

      // When: executeSearch is called
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: API should be called with trimmed query and limit
      expect(mockSearch).toHaveBeenCalledWith('migration', 25);
      expect(mockValidateQuery).toHaveBeenCalledWith('migration');
    });
  });

  describe('TR-SEARCH-006: executeSearch with empty query (no API call)', () => {
    it('should not call API when query is empty', async () => {
      // WHAT: Tests executeSearch skips API call for empty query
      // WHY: Prevents unnecessary API calls and errors
      // BREAKS IF: Empty queries trigger API calls, wasting resources
      // EXAMPLE: User clicks search with empty input, no API call made

      // Given: Hook with empty query
      const { result } = renderHook(() => useSearch());
      mockValidateQuery.mockReturnValue(false);

      // When: executeSearch is called with empty query
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: API should not be called
      expect(mockSearch).not.toHaveBeenCalled();
      expect(result.current.isLoading).toBe(false);
    });
  });

  describe('TR-SEARCH-007: executeSearch with whitespace-only query', () => {
    it('should not call API when query is whitespace-only', async () => {
      // WHAT: Tests executeSearch skips API call for whitespace-only query
      // WHY: Whitespace-only queries are effectively empty
      // BREAKS IF: Whitespace queries trigger API calls
      // EXAMPLE: User types only spaces, search doesn't execute

      // Given: Hook with whitespace-only query
      const { result } = renderHook(() => useSearch());
      mockValidateQuery.mockReturnValue(false);

      act(() => {
        result.current.setQuery('   ');
      });

      // When: executeSearch is called
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: API should not be called
      expect(mockValidateQuery).toHaveBeenCalledWith('');
      expect(mockSearch).not.toHaveBeenCalled();
    });
  });

  describe('TR-SEARCH-008: executeSearch updates results on success', () => {
    it('should update results array when API call succeeds', async () => {
      // WHAT: Tests executeSearch updates results state on successful API response
      // WHY: Results must be available for UI rendering
      // BREAKS IF: Results not updated, UI shows no results
      // EXAMPLE: API returns 3 results, hook updates results array

      // Given: Hook with valid query and mock API response
      const { result } = renderHook(() => useSearch());
      const mockResults: SearchResult[] = [
        { docId: 1, score: 0.95, snippet: 'Result 1' },
        { docId: 2, score: 0.85, snippet: 'Result 2' },
      ];
      const mockResponse: SearchResponse = {
        query: 'migration',
        results: mockResults,
      };
      mockSearch.mockResolvedValue(mockResponse);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('migration');
      });

      // When: executeSearch is called
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Results should be updated
      await waitFor(() => {
        expect(result.current.results).toEqual(mockResults);
        expect(result.current.results.length).toBe(2);
      });
    });
  });

  describe('TR-SEARCH-009: executeSearch updates metrics on success', () => {
    it('should update metrics with query, result count, and latency', async () => {
      // WHAT: Tests executeSearch calculates and updates metrics
      // WHY: Metrics provide search performance feedback
      // BREAKS IF: Metrics not calculated, user can't see search stats
      // EXAMPLE: Search completes in 150ms with 5 results, metrics show this

      // Given: Hook with valid query
      const { result } = renderHook(() => useSearch());
      const mockResponse: SearchResponse = {
        query: 'migration',
        results: [
          { docId: 1, score: 0.95, snippet: 'Result 1' },
          { docId: 2, score: 0.85, snippet: 'Result 2' },
        ],
      };
      mockSearch.mockResolvedValue(mockResponse);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('migration');
      });

      // When: executeSearch is called
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Metrics should be updated with correct values
      await waitFor(() => {
        expect(result.current.metrics).not.toBeNull();
        if (result.current.metrics) {
          expect(result.current.metrics.query).toBe('migration');
          expect(result.current.metrics.resultCount).toBe(2);
          expect(result.current.metrics.latency).toBeGreaterThanOrEqual(0);
        }
      });
    });
  });

  describe('TR-SEARCH-010: executeSearch calculates latency correctly', () => {
    it('should calculate latency as time difference between start and end', async () => {
      // WHAT: Tests latency calculation accuracy
      // WHY: Accurate latency helps diagnose performance issues
      // BREAKS IF: Latency incorrect, performance metrics misleading
      // EXAMPLE: API takes 200ms, latency should be ~200ms

      // Given: Hook with mock API that takes time
      const { result } = renderHook(() => useSearch());
      const mockResponse: SearchResponse = {
        query: 'test',
        results: [],
      };
      
      // Simulate API delay
      mockSearch.mockImplementation(() => 
        new Promise(resolve => setTimeout(() => resolve(mockResponse), 100))
      );
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called
      const startTime = Date.now();
      await act(async () => {
        await result.current.executeSearch();
      });
      const endTime = Date.now();

      // Then: Latency should be approximately correct
      await waitFor(() => {
        expect(result.current.metrics).not.toBeNull();
        if (result.current.metrics) {
          const calculatedLatency = result.current.metrics.latency;
          const actualLatency = endTime - startTime;
          // Allow 50ms tolerance for test execution overhead
          expect(calculatedLatency).toBeGreaterThanOrEqual(0);
          expect(Math.abs(calculatedLatency - actualLatency)).toBeLessThan(50);
        }
      });
    });
  });

  describe('TR-SEARCH-011: executeSearch handles NetworkError', () => {
    it('should set error message and clear results on NetworkError', async () => {
      // WHAT: Tests error handling for network failures
      // WHY: Network errors are common in microservices, must be handled gracefully
      // BREAKS IF: Network errors crash app or show no feedback
      // EXAMPLE: Backend unreachable, user sees "Cannot connect to server"

      // Given: Hook with valid query and network error
      const { result } = renderHook(() => useSearch());
      const networkError = new Error('Cannot connect to server');
      networkError.name = 'NetworkError';
      mockSearch.mockRejectedValue(networkError);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called and fails with NetworkError
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Error should be set, results cleared, metrics null
      await waitFor(() => {
        expect(result.current.error).toBe('Cannot connect to server');
        expect(result.current.results).toEqual([]);
        expect(result.current.metrics).toBeNull();
        expect(result.current.isLoading).toBe(false);
      });
    });
  });

  describe('TR-SEARCH-012: executeSearch handles TimeoutError', () => {
    it('should set error message and clear results on TimeoutError', async () => {
      // WHAT: Tests error handling for request timeouts
      // WHY: Timeouts prevent hanging requests in distributed systems
      // BREAKS IF: Timeouts not handled, UI hangs indefinitely
      // EXAMPLE: API takes >5s, user sees "Request timeout"

      // Given: Hook with valid query and timeout error
      const { result } = renderHook(() => useSearch());
      const timeoutError = new Error('Request timeout');
      timeoutError.name = 'TimeoutError';
      mockSearch.mockRejectedValue(timeoutError);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called and fails with TimeoutError
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Error should be set
      await waitFor(() => {
        expect(result.current.error).toBe('Request timeout');
        expect(result.current.results).toEqual([]);
        expect(result.current.metrics).toBeNull();
      });
    });
  });

  describe('TR-SEARCH-013: executeSearch handles HttpError', () => {
    it('should set error message and clear results on HttpError', async () => {
      // WHAT: Tests error handling for HTTP errors (4xx/5xx)
      // WHY: HTTP errors indicate backend issues, must be handled
      // BREAKS IF: HTTP errors crash app or show no feedback
      // EXAMPLE: Backend returns 500, user sees "Search failed with error 500"

      // Given: Hook with valid query and HTTP error
      const { result } = renderHook(() => useSearch());
      const httpError = new Error('Search failed with error 500');
      httpError.name = 'HttpError';
      // Create HttpError-like object for testing
      const httpErrorWithStatus = Object.assign(httpError, { status: 500 });
      mockSearch.mockRejectedValue(httpErrorWithStatus);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called and fails with HttpError
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Error should be set
      await waitFor(() => {
        expect(result.current.error).toBe('Search failed with error 500');
        expect(result.current.results).toEqual([]);
        expect(result.current.metrics).toBeNull();
      });
    });
  });

  describe('TR-SEARCH-014: executeSearch handles InvalidResponseError', () => {
    it('should set error message and clear results on InvalidResponseError', async () => {
      // WHAT: Tests error handling for invalid API responses
      // WHY: Invalid responses break type safety, must be caught
      // BREAKS IF: Invalid responses cause runtime errors
      // EXAMPLE: API returns malformed JSON, user sees "Invalid server response"

      // Given: Hook with valid query and invalid response error
      const { result } = renderHook(() => useSearch());
      const invalidError = new Error('Invalid server response');
      invalidError.name = 'InvalidResponseError';
      mockSearch.mockRejectedValue(invalidError);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called and fails with InvalidResponseError
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: Error should be set
      await waitFor(() => {
        expect(result.current.error).toBe('Invalid server response');
        expect(result.current.results).toEqual([]);
        expect(result.current.metrics).toBeNull();
      });
    });
  });

  describe('TR-SEARCH-015: executeSearch sets isLoading correctly', () => {
    it('should set isLoading to true during API call and false after completion', async () => {
      // WHAT: Tests loading state management during search
      // WHY: Loading state prevents duplicate requests and shows progress
      // BREAKS IF: Loading state incorrect, UI shows wrong state
      // EXAMPLE: Search starts, isLoading=true, completes, isLoading=false

      // Given: Hook with valid query and delayed API response
      const { result } = renderHook(() => useSearch());
      const mockResponse: SearchResponse = {
        query: 'test',
        results: [],
      };
      
      let resolvePromise: (value: SearchResponse) => void;
      const delayedPromise = new Promise<SearchResponse>((resolve) => {
        resolvePromise = resolve;
      });
      mockSearch.mockReturnValue(delayedPromise);
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called
      act(() => {
        result.current.executeSearch();
      });

      // Then: isLoading should be true during API call
      expect(result.current.isLoading).toBe(true);

      // When: API call completes
      await act(async () => {
        resolvePromise!(mockResponse);
        await delayedPromise;
      });

      // Then: isLoading should be false after completion
      await waitFor(() => {
        expect(result.current.isLoading).toBe(false);
      });
    });

    it('should set isLoading to false even when API call fails', async () => {
      // WHAT: Tests loading state is reset on error
      // WHY: Loading state must reset on error to allow retry
      // BREAKS IF: Loading stuck on true, user can't retry
      // EXAMPLE: Network error occurs, isLoading must become false

      // Given: Hook with valid query and error
      const { result } = renderHook(() => useSearch());
      mockSearch.mockRejectedValue(new Error('Network error'));
      mockValidateQuery.mockReturnValue(true);

      act(() => {
        result.current.setQuery('test');
      });

      // When: executeSearch is called and fails
      await act(async () => {
        await result.current.executeSearch();
      });

      // Then: isLoading should be false
      await waitFor(() => {
        expect(result.current.isLoading).toBe(false);
      });
    });
  });

  describe('TR-SEARCH-016: clearSearch resets all state', () => {
    it('should reset query, results, error, and metrics but not limit', () => {
      // WHAT: Tests clearSearch resets search-related state
      // WHY: Clear function allows users to start fresh searches
      // BREAKS IF: State not reset, old data persists
      // EXAMPLE: User clicks clear, query/results/error/metrics reset

      // Given: Hook with populated state
      const { result } = renderHook(() => useSearch());
      
      act(() => {
        result.current.setQuery('test query');
        result.current.setLimit(25);
      });

      // Set up some state (simulating previous search)
      act(() => {
        result.current.setQuery('previous');
      });

      // When: clearSearch is called
      act(() => {
        result.current.clearSearch();
      });

      // Then: Query, results, error, metrics should be reset, but limit preserved
      expect(result.current.query).toBe('');
      expect(result.current.results).toEqual([]);
      expect(result.current.error).toBeNull();
      expect(result.current.metrics).toBeNull();
      expect(result.current.limit).toBe(25); // Limit should NOT be reset
    });
  });

  describe('TR-SEARCH-017: clearSearch does not reset limit', () => {
    it('should preserve limit value when clearSearch is called', () => {
      // WHAT: Tests clearSearch preserves limit setting
      // WHY: Limit is a preference, not search-specific data
      // BREAKS IF: Limit reset, user loses preference
      // EXAMPLE: User sets limit to 25, clears search, limit stays 25

      // Given: Hook with custom limit
      const { result } = renderHook(() => useSearch());
      mockValidateLimit.mockReturnValue(true);

      act(() => {
        result.current.setLimit(25);
      });

      // When: clearSearch is called
      act(() => {
        result.current.clearSearch();
      });

      // Then: Limit should remain unchanged
      expect(result.current.limit).toBe(25);
    });
  });

  describe('TR-SEARCH-018: Multiple rapid searches maintain correct state', () => {
    it('should handle multiple rapid searches and maintain latest results', async () => {
      // WHAT: Tests hook handles concurrent/rapid searches correctly
      // WHY: Users may trigger multiple searches quickly, must handle gracefully
      // BREAKS IF: Race conditions cause incorrect results display
      // EXAMPLE: User types fast, multiple searches triggered, latest results shown

      // Given: Hook with multiple rapid search calls
      const { result } = renderHook(() => useSearch());
      mockValidateQuery.mockReturnValue(true);

      const response1: SearchResponse = {
        query: 'first',
        results: [{ docId: 1, score: 0.9, snippet: 'First result' }],
      };
      const response2: SearchResponse = {
        query: 'second',
        results: [{ docId: 2, score: 0.8, snippet: 'Second result' }],
      };

      // When: Multiple searches are triggered rapidly
      act(() => {
        result.current.setQuery('first');
      });

      const promise1 = act(async () => {
        mockSearch.mockResolvedValueOnce(response1);
        await result.current.executeSearch();
      });

      act(() => {
        result.current.setQuery('second');
      });

      const promise2 = act(async () => {
        mockSearch.mockResolvedValueOnce(response2);
        await result.current.executeSearch();
      });

      await Promise.all([promise1, promise2]);

      // Then: Latest search results should be displayed
      await waitFor(() => {
        expect(result.current.results).toEqual(response2.results);
        expect(result.current.metrics?.query).toBe('second');
      });
    });
  });
});
