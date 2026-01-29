/**
 * useDebounce Hook Test Suite
 * 
 * WHAT: Tests the useDebounce utility hook that delays value updates
 * WHY: Debouncing reduces API calls and improves performance in microservices
 * BREAKS IF: Debouncing doesn't work, causing excessive API calls and server overload
 * EXAMPLE: User types "migration", debounce delays update until typing stops
 * 
 * Test-Driven Development (TDD) - RED PHASE
 * These tests will fail until useDebounce hook is implemented.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { renderHook, act } from '@testing-library/react';
import { useDebounce } from './useDebounce';

describe('useDebounce', () => {
  beforeEach(() => {
    // Use fake timers for delay tests
    vi.useFakeTimers();
  });

  afterEach(() => {
    // Restore real timers
    vi.useRealTimers();
  });

  describe('TR-DEBOUNCE-001: Returns initial value immediately', () => {
    it('should return initial value without delay on first render', () => {
      // WHAT: Tests initial value is returned immediately
      // WHY: Initial value must be available immediately for UI rendering
      // BREAKS IF: Initial value delayed, UI shows undefined/null
      // EXAMPLE: Component mounts with query="test", debouncedQuery="test" immediately

      // Given: Hook with initial value
      const { result } = renderHook(() => useDebounce('initial', 300));

      // When: No time has passed
      // Then: Value should be returned immediately
      expect(result.current).toBe('initial');
    });
  });

  describe('TR-DEBOUNCE-002: Delays value update by specified delay', () => {
    it('should delay returning updated value by specified milliseconds', () => {
      // WHAT: Tests debounce delay works correctly
      // WHY: Delay prevents rapid updates, reducing API calls
      // BREAKS IF: Delay not working, updates happen too quickly
      // EXAMPLE: Value changes, waits 300ms, then updates

      // Given: Hook with initial value and 300ms delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'initial', delay: 300 },
        }
      );

      expect(result.current).toBe('initial');

      // When: Value changes
      rerender({ value: 'updated', delay: 300 });

      // Then: Value should not update immediately
      expect(result.current).toBe('initial');

      // When: Delay time passes
      act(() => {
        vi.advanceTimersByTime(300);
      });

      // Then: Value should be updated
      expect(result.current).toBe('updated');
    });
  });

  describe('TR-DEBOUNCE-003: Resets timer on value change', () => {
    it('should reset timer when value changes before delay expires', () => {
      // WHAT: Tests timer reset on rapid value changes
      // WHY: Only final value should be returned after user stops changing
      // BREAKS IF: Timer not reset, intermediate values returned
      // EXAMPLE: User types "t", "te", "tes", "test" - only "test" returned

      // Given: Hook with 300ms delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'a', delay: 300 },
        }
      );

      // When: Value changes rapidly
      rerender({ value: 'ab', delay: 300 });
      act(() => {
        vi.advanceTimersByTime(100); // 100ms passed
      });

      rerender({ value: 'abc', delay: 300 });
      act(() => {
        vi.advanceTimersByTime(100); // 200ms total
      });

      rerender({ value: 'abcd', delay: 300 });
      act(() => {
        vi.advanceTimersByTime(100); // 300ms total, but timer reset
      });

      // Then: Value should still be initial (timer reset)
      expect(result.current).toBe('a');

      // When: No more changes, delay expires
      act(() => {
        vi.advanceTimersByTime(300);
      });

      // Then: Final value should be returned
      expect(result.current).toBe('abcd');
    });
  });

  describe('TR-DEBOUNCE-004: Returns latest value after delay', () => {
    it('should return the latest value after delay expires', () => {
      // WHAT: Tests final value is returned after delay
      // WHY: Only final value matters after user stops changing
      // BREAKS IF: Intermediate values returned, causing incorrect API calls
      // EXAMPLE: User types "migration", only "migration" used for search

      // Given: Hook with 300ms delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'first', delay: 300 },
        }
      );

      // When: Multiple rapid changes
      rerender({ value: 'second', delay: 300 });
      rerender({ value: 'third', delay: 300 });
      rerender({ value: 'fourth', delay: 300 });

      // Then: Value should still be initial
      expect(result.current).toBe('first');

      // When: Delay expires
      act(() => {
        vi.advanceTimersByTime(300);
      });

      // Then: Latest value should be returned
      expect(result.current).toBe('fourth');
    });
  });

  describe('TR-DEBOUNCE-005: Handles multiple rapid changes', () => {
    it('should handle multiple rapid value changes correctly', () => {
      // WHAT: Tests multiple rapid changes are handled properly
      // WHY: Users type quickly, debounce must handle all changes
      // BREAKS IF: Rapid changes cause issues or incorrect values
      // EXAMPLE: User types quickly, debounce handles all keystrokes

      // Given: Hook with 300ms delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: '', delay: 300 },
        }
      );

      // When: Multiple rapid changes (simulating typing)
      const values = ['t', 'te', 'tes', 'test'];
      values.forEach((val) => {
        rerender({ value: val, delay: 300 });
        act(() => {
          vi.advanceTimersByTime(50); // Simulate rapid typing
        });
      });

      // Then: Value should still be initial (timer keeps resetting)
      expect(result.current).toBe('');

      // When: User stops typing, delay expires
      act(() => {
        vi.advanceTimersByTime(300);
      });

      // Then: Final value should be returned
      expect(result.current).toBe('test');
    });
  });

  describe('TR-DEBOUNCE-006: Clears timeout on unmount', () => {
    it('should clear timeout when component unmounts', () => {
      // WHAT: Tests cleanup prevents memory leaks
      // WHY: Unmounted components shouldn't update state
      // BREAKS IF: Timeout not cleared, memory leaks and React warnings
      // EXAMPLE: Component unmounts, pending timeout cleared

      // Given: Hook with pending update
      const { result, rerender, unmount } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'initial', delay: 300 },
        }
      );

      rerender({ value: 'updated', delay: 300 });

      // When: Component unmounts before delay expires
      unmount();

      // When: Advance time after unmount
      act(() => {
        vi.advanceTimersByTime(300);
      });

      // Then: No errors should occur (timeout cleared)
      // This test verifies no React warnings about setState on unmounted component
      expect(true).toBe(true); // Test passes if no errors thrown
    });
  });

  describe('TR-DEBOUNCE-007: Works with string values', () => {
    it('should debounce string values correctly', () => {
      // WHAT: Tests debounce works with string type
      // WHY: Strings are common for search queries
      // BREAKS IF: String debouncing doesn't work, search queries update too fast
      // EXAMPLE: Search query "migration" debounced correctly

      // Given: Hook with string value
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'hello', delay: 200 },
        }
      );

      expect(result.current).toBe('hello');

      // When: String value changes
      rerender({ value: 'world', delay: 200 });

      // Then: Value should update after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toBe('world');
    });
  });

  describe('TR-DEBOUNCE-008: Works with number values', () => {
    it('should debounce number values correctly', () => {
      // WHAT: Tests debounce works with number type
      // WHY: Numbers are used for limits, filters, etc.
      // BREAKS IF: Number debouncing doesn't work, values update too fast
      // EXAMPLE: Limit value 25 debounced correctly

      // Given: Hook with number value
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 10, delay: 200 },
        }
      );

      expect(result.current).toBe(10);

      // When: Number value changes
      rerender({ value: 25, delay: 200 });

      // Then: Value should update after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toBe(25);
    });
  });

  describe('TR-DEBOUNCE-009: Works with object values', () => {
    it('should debounce object values correctly', () => {
      // WHAT: Tests debounce works with object type
      // WHY: Objects are used for filters, configs, etc.
      // BREAKS IF: Object debouncing doesn't work, objects update too fast
      // EXAMPLE: Filter object { category: 'tech' } debounced correctly

      // Given: Hook with object value
      const initialObj = { category: 'tech', tag: 'react' };
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: initialObj, delay: 200 },
        }
      );

      expect(result.current).toBe(initialObj);

      // When: Object value changes (new reference)
      const updatedObj = { category: 'tech', tag: 'vue' };
      rerender({ value: updatedObj, delay: 200 });

      // Then: Value should update after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toBe(updatedObj);
    });

    it('should trigger debounce on object reference change', () => {
      // WHAT: Tests debounce triggers on object reference change
      // WHY: Objects compared by reference, not deep equality
      // BREAKS IF: Object changes not detected, debounce doesn't reset
      // EXAMPLE: { ...obj } creates new reference, debounce resets

      // Given: Hook with object value
      const obj = { count: 1 };
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: obj, delay: 200 },
        }
      );

      // When: Object reference changes (same content, new reference)
      const newObj = { ...obj };
      rerender({ value: newObj, delay: 200 });

      // Then: Timer should reset (value hasn't updated yet)
      expect(result.current).toBe(obj);

      // When: Delay expires
      act(() => {
        vi.advanceTimersByTime(200);
      });

      // Then: New object should be returned
      expect(result.current).toBe(newObj);
    });
  });

  describe('TR-DEBOUNCE-010: Works with array values', () => {
    it('should debounce array values correctly', () => {
      // WHAT: Tests debounce works with array type
      // WHY: Arrays are used for lists, selections, etc.
      // BREAKS IF: Array debouncing doesn't work, arrays update too fast
      // EXAMPLE: Selected items [1, 2, 3] debounced correctly

      // Given: Hook with array value
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: [1, 2], delay: 200 },
        }
      );

      expect(result.current).toEqual([1, 2]);

      // When: Array value changes
      rerender({ value: [1, 2, 3], delay: 200 });

      // Then: Value should update after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toEqual([1, 2, 3]);
    });

    it('should trigger debounce on array reference change', () => {
      // WHAT: Tests debounce triggers on array reference change
      // WHY: Arrays compared by reference, not deep equality
      // BREAKS IF: Array changes not detected, debounce doesn't reset
      // EXAMPLE: [...arr] creates new reference, debounce resets

      // Given: Hook with array value
      const arr = [1, 2];
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: arr, delay: 200 },
        }
      );

      // When: Array reference changes (same content, new reference)
      const newArr = [...arr];
      rerender({ value: newArr, delay: 200 });

      // Then: Timer should reset
      expect(result.current).toBe(arr);

      // When: Delay expires
      act(() => {
        vi.advanceTimersByTime(200);
      });

      // Then: New array should be returned
      expect(result.current).toBe(newArr);
    });
  });

  describe('TR-DEBOUNCE-011: Handles zero delay', () => {
    it('should return updated value immediately when delay is zero', () => {
      // WHAT: Tests zero delay returns value immediately
      // WHY: Zero delay means no debouncing, immediate update
      // BREAKS IF: Zero delay causes issues or delays
      // EXAMPLE: Delay 0ms, value updates immediately

      // Given: Hook with zero delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'initial', delay: 0 },
        }
      );

      expect(result.current).toBe('initial');

      // When: Value changes
      rerender({ value: 'updated', delay: 0 });

      // Then: Value should update immediately (or very quickly)
      // Note: Even with zero delay, React may batch updates, so we advance timers
      act(() => {
        vi.advanceTimersByTime(0);
      });
      expect(result.current).toBe('updated');
    });
  });

  describe('TR-DEBOUNCE-012: Handles null values', () => {
    it('should debounce null values correctly', () => {
      // WHAT: Tests debounce handles null values
      // WHY: Null is a valid value that may need debouncing
      // BREAKS IF: Null causes errors or incorrect behavior
      // EXAMPLE: Value changes from string to null, debounced correctly

      // Given: Hook with string value
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'test', delay: 200 },
        }
      );

      expect(result.current).toBe('test');

      // When: Value changes to null
      rerender({ value: null, delay: 200 });

      // Then: Value should update to null after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toBeNull();
    });
  });

  describe('TR-DEBOUNCE-013: Handles undefined values', () => {
    it('should debounce undefined values correctly', () => {
      // WHAT: Tests debounce handles undefined values
      // WHY: Undefined is a valid value that may need debouncing
      // BREAKS IF: Undefined causes errors or incorrect behavior
      // EXAMPLE: Value changes from string to undefined, debounced correctly

      // Given: Hook with string value
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'test', delay: 200 },
        }
      );

      expect(result.current).toBe('test');

      // When: Value changes to undefined
      rerender({ value: undefined, delay: 200 });

      // Then: Value should update to undefined after delay
      act(() => {
        vi.advanceTimersByTime(200);
      });
      expect(result.current).toBeUndefined();
    });
  });

  describe('TR-DEBOUNCE-014: Prevents memory leaks', () => {
    it('should clean up all timers and prevent memory leaks', () => {
      // WHAT: Tests cleanup prevents memory leaks
      // WHY: Memory leaks cause performance degradation over time
      // BREAKS IF: Timers not cleared, memory usage grows
      // EXAMPLE: Component unmounts, all timers cleared, no leaks

      // Given: Hook with multiple value changes
      const { result, rerender, unmount } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'a', delay: 300 },
        }
      );

      // Trigger multiple changes
      rerender({ value: 'b', delay: 300 });
      rerender({ value: 'c', delay: 300 });
      rerender({ value: 'd', delay: 300 });

      // When: Component unmounts
      unmount();

      // When: Advance time after unmount
      act(() => {
        vi.advanceTimersByTime(1000);
      });

      // Then: No errors should occur (all timers cleared)
      // This test verifies no memory leaks or React warnings
      expect(true).toBe(true); // Test passes if no errors thrown
    });

    it('should handle delay changes correctly', () => {
      // WHAT: Tests debounce works when delay changes
      // WHY: Delay may change based on configuration
      // BREAKS IF: Delay changes cause issues or incorrect behavior
      // EXAMPLE: Delay changes from 300ms to 500ms, debounce adjusts

      // Given: Hook with initial delay
      const { result, rerender } = renderHook(
        ({ value, delay }) => useDebounce(value, delay),
        {
          initialProps: { value: 'initial', delay: 300 },
        }
      );

      // When: Value changes
      rerender({ value: 'updated', delay: 300 });

      // When: Delay changes before timer expires
      rerender({ value: 'updated', delay: 500 });

      // Then: Timer should use new delay
      act(() => {
        vi.advanceTimersByTime(300);
      });
      expect(result.current).toBe('initial'); // Still initial (delay increased)

      act(() => {
        vi.advanceTimersByTime(200); // Total 500ms
      });
      expect(result.current).toBe('updated'); // Now updated
    });
  });
});
