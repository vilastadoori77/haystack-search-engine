# useDebounce Hook Specification

**Project:** Haystack Search Engine - React Frontend  
**Component:** useDebounce Custom Hook  
**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Draft Specification  
**Author:** Technical Specification Team  

---

## Document Control

| Version | Date | Author | Status | Changes |
|---------|------|--------|--------|---------|
| 1.0 | 2026-01-28 | Spec Team | Draft | Initial specification |

---

## Table of Contents

1. [Overview](#1-overview)
2. [Interface Definition](#2-interface-definition)
3. [Functional Requirements](#3-functional-requirements)
4. [Debouncing Algorithm](#4-debouncing-algorithm)
5. [Implementation Details](#5-implementation-details)
6. [Cursor Implementation Notes](#6-cursor-implementation-notes)
7. [Test Requirements](#7-test-requirements)
8. [Usage Examples](#8-usage-examples)

---

## 1. Overview

### 1.1 Purpose

The `useDebounce` hook is a generic utility that delays updating a value until a specified delay period has passed without the value changing. This reduces the frequency of updates and can be used to reduce API calls, improve performance, and enhance user experience.

### 1.2 Responsibilities

- Delay value updates by specified milliseconds
- Reset timer on each value change
- Return debounced value of same type as input
- Clean up timers on unmount
- Prevent memory leaks

### 1.3 Non-Responsibilities

- Value validation
- Value transformation
- API calls (used in conjunction with other hooks)
- UI rendering

### 1.4 Use Cases

- **Search Input Debouncing:** Delay search API calls until user stops typing
- **Form Input Debouncing:** Delay validation until user stops typing
- **Resize Event Debouncing:** Delay resize handler execution
- **Scroll Event Debouncing:** Delay scroll handler execution

---

## 2. Interface Definition

### 2.1 Hook Signature

```typescript
function useDebounce<T>(value: T, delay: number): T
```

### 2.2 Type Parameters

- `T`: Generic type parameter representing the value type
  - Can be: `string`, `number`, `object`, `array`, etc.
  - Return type matches input type

### 2.3 Parameters

- `value` (T): The value to debounce
- `delay` (number): Delay in milliseconds before updating debounced value

### 2.4 Return Value

- Returns: `T` - The debounced value (same type as input)

---

## 3. Functional Requirements

### 3.1 Type Safety Requirements

**FR-DEBOUNCE-TYPE-001:** SHALL accept any value type (generic T)  
**FR-DEBOUNCE-TYPE-002:** SHALL return value of same type as input  
**FR-DEBOUNCE-TYPE-003:** SHALL preserve value type through debouncing  
**FR-DEBOUNCE-TYPE-004:** SHALL work with primitive types (string, number, boolean)  
**FR-DEBOUNCE-TYPE-005:** SHALL work with complex types (object, array)  

### 3.2 Initial Value Requirements

**FR-DEBOUNCE-INIT-001:** SHALL return initial value immediately  
**FR-DEBOUNCE-INIT-002:** SHALL NOT delay initial value  
**FR-DEBOUNCE-INIT-003:** SHALL return first value without delay  

### 3.3 Debouncing Requirements

**FR-DEBOUNCE-DELAY-001:** SHALL accept delay in milliseconds  
**FR-DEBOUNCE-DELAY-002:** SHALL delay returning updated value by delay  
**FR-DEBOUNCE-DELAY-003:** SHALL reset timer on each value change  
**FR-DEBOUNCE-DELAY-004:** SHALL return latest value after delay expires  
**FR-DEBOUNCE-DELAY-005:** SHALL return only final value after rapid changes  
**FR-DEBOUNCE-DELAY-006:** SHALL cancel previous timer on value change  

### 3.4 Cleanup Requirements

**FR-DEBOUNCE-CLEANUP-001:** SHALL clear timeout on unmount  
**FR-DEBOUNCE-CLEANUP-002:** SHALL prevent memory leaks  
**FR-DEBOUNCE-CLEANUP-003:** SHALL handle multiple rapid updates  
**FR-DEBOUNCE-CLEANUP-004:** SHALL not update state after unmount  

### 3.5 Edge Case Requirements

**FR-DEBOUNCE-EDGE-001:** SHALL handle zero delay (return immediately)  
**FR-DEBOUNCE-EDGE-002:** SHALL handle very large delays  
**FR-DEBOUNCE-EDGE-003:** SHALL handle null/undefined values (passes through as-is, consuming code should validate if needed)  
**FR-DEBOUNCE-EDGE-004:** SHALL handle object reference changes  
**FR-DEBOUNCE-EDGE-005:** SHALL handle array reference changes  

---

## 4. Debouncing Algorithm

### 4.1 Algorithm Flow

```
Input: value (T), delay (number)
Output: debouncedValue (T)

1. Return initial value immediately (first render)
2. On value change:
   a. Clear existing timeout (if any)
   b. Set new timeout for delay milliseconds
   c. After delay expires, update debouncedValue
3. On unmount:
   a. Clear any pending timeout
   b. Prevent state updates
```

### 4.2 Timing Diagram

**Scenario: User types "test" rapidly**

```
Time  | Input Value | Timer Status        | Debounced Value
------|-------------|---------------------|----------------
0ms   | ""          | -                   | "" (initial)
10ms  | "t"         | Start 300ms timer  | ""
50ms  | "te"        | Cancel, start 300ms | ""
100ms | "tes"       | Cancel, start 300ms | ""
150ms | "test"      | Cancel, start 300ms | ""
450ms | "test"      | Timer expires       | "test" ✅
```

**Result:** Only one update after user stops typing

### 4.3 Multiple Rapid Changes

**Scenario: Value changes 5 times in 100ms with 300ms delay**

```
Change 1 (0ms)   → Timer 1 starts (expires at 300ms)
Change 2 (20ms)  → Timer 1 cancelled, Timer 2 starts (expires at 320ms)
Change 3 (40ms)  → Timer 2 cancelled, Timer 3 starts (expires at 340ms)
Change 4 (60ms)  → Timer 3 cancelled, Timer 4 starts (expires at 360ms)
Change 5 (80ms)  → Timer 4 cancelled, Timer 5 starts (expires at 380ms)
... (no more changes)
380ms            → Timer 5 expires, debouncedValue updates
```

**Result:** Only final value (from change 5) is returned

### 4.4 Object and Array Handling

**Important Note:**
- useDebounce compares values by reference, not deep equality
- New object/array reference triggers debounce reset
- Example: `{...obj}` creates new reference, triggers debounce
- For deep equality comparison, use custom comparison hook

---

## 5. Implementation Details

### 5.1 Hook Structure

```typescript
import { useState, useEffect } from 'react';

/**
 * Debounces a value, delaying updates until the value has not changed
 * for the specified delay period.
 * 
 * @template T - The type of the value to debounce
 * @param value - The value to debounce
 * @param delay - Delay in milliseconds before updating debounced value
 * @returns The debounced value (same type as input)
 * 
 * @example
 * const [query, setQuery] = useState('');
 * const debouncedQuery = useDebounce(query, 300);
 * 
 * // query changes immediately
 * // debouncedQuery changes 300ms after query stops changing
 */
export function useDebounce<T>(value: T, delay: number): T {
  const [debouncedValue, setDebouncedValue] = useState<T>(value);

  useEffect(() => {
    // Set up timer to update debounced value after delay
    const timer = setTimeout(() => {
      setDebouncedValue(value);
    }, delay);

    // Cleanup: cancel timer if value changes before delay expires
    return () => {
      clearTimeout(timer);
    };
  }, [value, delay]);

  return debouncedValue;
}
```

### 5.2 Dependencies

**External Dependencies:**
- `react` (useState, useEffect)

**Internal Dependencies:**
- None (self-contained utility hook)

### 5.3 Performance Considerations

**Timer Management:**
- Clear previous timer on each value change
- Prevent multiple timers from running
- Clean up on unmount

**Re-render Optimization:**
- Only updates debouncedValue when timer expires
- Reduces unnecessary re-renders in consuming components
- Delay can be adjusted based on use case

**Memory Management:**
- Cleanup function in useEffect
- Clear timeout on unmount
- Prevent memory leaks

**Timer Granularity:**
- Browser timers have ~4-16ms granularity
- Delays <16ms may not behave precisely
- Recommended minimum delay: 50ms for predictable behavior

---

## 6. Cursor Implementation Notes

When generating code from this specification:
- Use exact type signature: `function useDebounce<T>(value: T, delay: number): T`
- Follow implementation structure from Section 5.1
- Implement all test cases from Section 6
- Use setTimeout with cleanup in useEffect
- Include JSDoc comments with @template, @param, @returns
- Include usage example in JSDoc

---

## 7. Test Requirements

### 7.1 Test Coverage Requirements

**TR-DEBOUNCE-001:** Test returns initial value immediately  
**TR-DEBOUNCE-002:** Test delays value update by delay milliseconds  
**TR-DEBOUNCE-003:** Test resets timer on value change  
**TR-DEBOUNCE-004:** Test returns latest value after delay  
**TR-DEBOUNCE-005:** Test handles multiple rapid changes  
**TR-DEBOUNCE-006:** Test clears timeout on unmount  
**TR-DEBOUNCE-007:** Test works with string values  
**TR-DEBOUNCE-008:** Test works with number values  
**TR-DEBOUNCE-009:** Test works with object values  
**TR-DEBOUNCE-010:** Test works with array values  
**TR-DEBOUNCE-011:** Test handles zero delay  
**TR-DEBOUNCE-012:** Test handles null values  
**TR-DEBOUNCE-013:** Test handles undefined values  
**TR-DEBOUNCE-014:** Test prevents memory leaks  

### 7.2 Test Structure

**Given-When-Then Format:**
- Given: Initial value and delay
- When: Value changes or time advances
- Then: Expected debounced value

**Mock Requirements:**
- Use fake timers (vi.useFakeTimers)
- Control time advancement
- Verify timer cleanup

**Test Isolation:**
- Each test is independent
- Reset timers between tests
- Clean up timeouts

---

## 8. Usage Examples

### 8.1 Basic Usage (String)

```typescript
import { useState } from 'react';
import { useDebounce } from '../hooks/useDebounce';

function SearchComponent() {
  const [query, setQuery] = useState('');
  const debouncedQuery = useDebounce(query, 300);

  // query updates immediately on user input
  // debouncedQuery updates 300ms after user stops typing

  useEffect(() => {
    if (debouncedQuery) {
      // Perform search with debouncedQuery
      performSearch(debouncedQuery);
    }
  }, [debouncedQuery]);

  return (
    <input
      value={query}
      onChange={(e) => setQuery(e.target.value)}
      placeholder="Search..."
    />
  );
}
```

### 8.2 Usage with useSearch Hook

```typescript
import { useEffect } from 'react';
import { useDebounce } from '../hooks/useDebounce';
import { useSearch } from '../hooks/useSearch';

function AutoSearchComponent() {
  const { query, setQuery, executeSearch } = useSearch();
  const debouncedQuery = useDebounce(query, 500);

  // Auto-execute search when debounced query changes
  useEffect(() => {
    if (debouncedQuery.trim()) {
      executeSearch();
    }
  }, [debouncedQuery, executeSearch]);

  return (
    <input
      value={query}
      onChange={(e) => setQuery(e.target.value)}
    />
  );
}
```

### 8.3 Usage with Number Values

```typescript
const [limit, setLimit] = useState(10);
const debouncedLimit = useDebounce(limit, 200);

// Use debouncedLimit for API calls
```

### 8.4 Usage with Object Values

```typescript
const [filters, setFilters] = useState({ category: '', tag: '' });
const debouncedFilters = useDebounce(filters, 400);

// Use debouncedFilters for API calls
```

### 8.5 Different Delay Values

```typescript
// Fast debounce (100ms) - for UI updates
const debouncedQuery = useDebounce(query, 100);

// Medium debounce (300ms) - for search suggestions
const debouncedQuery = useDebounce(query, 300);

// Slow debounce (500ms) - for API calls
const debouncedQuery = useDebounce(query, 500);
```

---

## Appendices

### Appendix A: Performance Impact

**Without Debouncing:**
- User types "test" → 4 API calls (t, te, tes, test)
- High server load
- Unnecessary network traffic

**With Debouncing (300ms):**
- User types "test" → 1 API call (after 300ms delay)
- 75% reduction in API calls
- Better user experience
- Reduced server load

### Appendix B: Recommended Delay Values

| Use Case | Recommended Delay | Rationale |
|----------|-------------------|------------|
| Search API calls | 300-500ms | Balance responsiveness and efficiency |
| Form validation | 300ms | Immediate feedback without being annoying |
| UI updates | 100-200ms | Fast enough for smooth UX |
| Resize handlers | 150ms | Smooth resizing without lag |
| Scroll handlers | 100ms | Responsive scrolling |

### Appendix C: Comparison with Alternatives

**Alternative 1: No Debouncing**
- Pros: Immediate updates
- Cons: Too many API calls, poor performance

**Alternative 2: Throttling**
- Pros: Limits update frequency
- Cons: May miss final value, less precise

**Alternative 3: Debouncing (this hook)**
- Pros: Waits for final value, reduces calls
- Cons: Slight delay (acceptable for most cases)

### Appendix D: Implementation Review Checklist

Before marking useDebounce as complete:
- [ ] All FR-DEBOUNCE requirements implemented (Section 3)
- [ ] All 14 test cases pass (Section 7)
- [ ] Zero TypeScript errors
- [ ] Generic type parameter <T> works correctly
- [ ] Cleanup function clears timeout
- [ ] Code follows structure from Section 5.1
- [ ] JSDoc comments with @template present

---

**END OF SPECIFICATION**

**Document ID:** SPEC-USE-DEBOUNCE-001  
**Version:** 1.0  
**Status:** DRAFT  
**Next Document:** Implementation Phase
