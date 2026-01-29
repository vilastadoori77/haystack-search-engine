# useSearch Hook Specification

**Project:** Haystack Search Engine - React Frontend  
**Component:** useSearch Custom Hook  
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
3. [State Management](#3-state-management)
4. [Functional Requirements](#4-functional-requirements)
5. [Error Handling](#5-error-handling)
6. [Implementation Details](#6-implementation-details)
7. [Test Requirements](#7-test-requirements)
8. [Microservices Considerations](#8-microservices-considerations)
9. [Cursor Implementation Notes](#9-cursor-implementation-notes)

---

## 1. Overview

### 1.1 Purpose

The `useSearch` hook encapsulates all search-related state management and API interaction logic, extracting it from the App component to improve maintainability, testability, and reusability.

### 1.2 Responsibilities

- Manage search query state
- Manage search limit state (1-50)
- Manage search results state
- Manage search metrics state
- Manage loading state
- Manage error state
- Execute search API calls
- Calculate API latency
- Handle all search-related errors gracefully
- Provide search state manipulation functions

### 1.3 Non-Responsibilities

- UI rendering
- User input handling
- Component lifecycle management
- Backend API implementation
- Data persistence

---

## 2. Interface Definition

### 2.1 Hook Signature

```typescript
function useSearch(): UseSearchReturn
```

### 2.2 Return Type

```typescript
interface UseSearchReturn {
  // State
  query: string;
  limit: number;
  results: SearchResult[];
  metrics: SearchMetrics | null;
  error: string | null;
  isLoading: boolean;

  // Actions
  setQuery: (query: string) => void;
  setLimit: (limit: number) => void;
  executeSearch: () => Promise<void>;
  clearSearch: () => void;
}
```

### 2.3 Type Definitions

```typescript
import type { SearchResult, SearchResponse } from '../types/search';

interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number; // milliseconds
}
```

---

## 3. State Management

### 3.1 Initial State

```typescript
query: ''           // Empty string
limit: 10           // Default to 10 results
results: []         // Empty array
metrics: null       // No metrics initially
error: null         // No errors initially
isLoading: false    // Not loading initially
```

### 3.2 State Transitions

**Query State:**
- `setQuery('new query')` → `query = 'new query'`
- `clearSearch()` → `query = ''`

**Limit State:**
- `setLimit(25)` → `limit = 25`
- `setLimit(0)` → Validation error, limit unchanged
- `setLimit(51)` → Validation error, limit unchanged

**Results State:**
- `executeSearch()` success → `results = SearchResult[]`
- `executeSearch()` failure → `results = []`
- `clearSearch()` → `results = []`

**Metrics State:**
- `executeSearch()` success → `metrics = { query, resultCount, latency }`
- `executeSearch()` failure → `metrics = null`
- `clearSearch()` → `metrics = null`

**Error State:**
- `executeSearch()` success → `error = null`
- `executeSearch()` failure → `error = 'user-friendly message'`
- `clearSearch()` → `error = null`

**Loading State:**
- `executeSearch()` starts → `isLoading = true`
- `executeSearch()` completes → `isLoading = false`
- `clearSearch()` → `isLoading = false` (if was true)

---

## 4. Functional Requirements

### 4.1 State Management Requirements

**FR-SEARCH-STATE-001:** SHALL initialize query as empty string  
**FR-SEARCH-STATE-002:** SHALL initialize limit as 10  
**FR-SEARCH-STATE-003:** SHALL initialize results as empty array  
**FR-SEARCH-STATE-004:** SHALL initialize metrics as null  
**FR-SEARCH-STATE-005:** SHALL initialize error as null  
**FR-SEARCH-STATE-006:** SHALL initialize isLoading as false  

### 4.2 Query Management Requirements

**FR-SEARCH-QUERY-001:** `setQuery()` SHALL update query state immediately  
**FR-SEARCH-QUERY-002:** `setQuery()` SHALL accept any string value  
**FR-SEARCH-QUERY-003:** `setQuery()` SHALL NOT trim whitespace automatically  

**Note:** setQuery() accepts the value as-is without trimming to preserve user input exactly as typed. However, executeSearch() trims the query before validation to ignore leading/trailing whitespace when determining if query is empty.

**FR-SEARCH-QUERY-004:** `executeSearch()` SHALL trim query before validation  
**FR-SEARCH-QUERY-005:** `executeSearch()` SHALL validate query is non-empty  
**FR-SEARCH-QUERY-006:** `executeSearch()` SHALL NOT execute if query is empty/whitespace  

### 4.3 Limit Management Requirements

**FR-SEARCH-LIMIT-001:** `setLimit()` SHALL update limit state immediately  
**FR-SEARCH-LIMIT-002:** `setLimit()` SHALL validate limit is between 1 and 50  
**FR-SEARCH-LIMIT-003:** `setLimit()` SHALL NOT update if limit < 1  
**FR-SEARCH-LIMIT-004:** `setLimit()` SHALL NOT update if limit > 50  
**FR-SEARCH-LIMIT-005:** `setLimit()` SHALL accept integer values only  
**FR-SEARCH-LIMIT-006:** `executeSearch()` SHALL use current limit value  

### 4.4 Search Execution Requirements

**FR-SEARCH-EXEC-001:** `executeSearch()` SHALL set isLoading to true before API call  
**FR-SEARCH-EXEC-002:** `executeSearch()` SHALL clear error state before API call  
**FR-SEARCH-EXEC-003:** `executeSearch()` SHALL clear previous results before API call  
**FR-SEARCH-EXEC-004:** `executeSearch()` SHALL track start time before API call  
**FR-SEARCH-EXEC-005:** `executeSearch()` SHALL call `searchApi.search(query, limit)`  
**FR-SEARCH-EXEC-006:** `executeSearch()` SHALL calculate latency (endTime - startTime)  
**FR-SEARCH-EXEC-007:** `executeSearch()` SHALL update results on success  
**FR-SEARCH-EXEC-008:** `executeSearch()` SHALL update metrics on success  
**FR-SEARCH-EXEC-009:** `executeSearch()` SHALL set isLoading to false after completion  
**FR-SEARCH-EXEC-010:** `executeSearch()` SHALL handle errors gracefully (no exceptions)  

### 4.5 Metrics Requirements

**FR-SEARCH-METRICS-001:** SHALL calculate latency in milliseconds  
**FR-SEARCH-METRICS-002:** SHALL include query from API response  
**FR-SEARCH-METRICS-003:** SHALL include resultCount from results array length  
**FR-SEARCH-METRICS-004:** SHALL set metrics to null on error  
**FR-SEARCH-METRICS-005:** SHALL set metrics to null on clear  

### 4.6 Clear Functionality Requirements

**FR-SEARCH-CLEAR-001:** `clearSearch()` SHALL reset query to empty string  
**FR-SEARCH-CLEAR-002:** `clearSearch()` SHALL reset results to empty array  
**FR-SEARCH-CLEAR-003:** `clearSearch()` SHALL reset error to null  
**FR-SEARCH-CLEAR-004:** `clearSearch()` SHALL reset metrics to null  
**FR-SEARCH-CLEAR-005:** `clearSearch()` SHALL NOT reset limit  
**FR-SEARCH-CLEAR-006:** `clearSearch()` SHALL NOT reset isLoading  
**FR-SEARCH-CLEAR-007:** `clearSearch()` SHOULD NOT cancel in-flight API requests (implementation note: requests complete but results are ignored if search was cleared)  

---

## 5. Error Handling

### 5.1 Error Types and Messages

**NetworkError:**
- **Message:** "Cannot connect to server"
- **Action:** Set error, clear results, clear metrics

**TimeoutError:**
- **Message:** "Request timeout"
- **Action:** Set error, clear results, clear metrics

**HttpError:**
- **Message:** "Search failed with error {status}"
- **Action:** Set error, clear results, clear metrics

**InvalidResponseError:**
- **Message:** "Invalid server response"
- **Action:** Set error, clear results, clear metrics

**Validation Error (empty query):**
- **Message:** None (function returns early, no error set)
- **Action:** Do nothing, don't call API

**Validation Error (invalid limit):**
- **Message:** None (validation in setLimit)
- **Action:** Prevent limit update

### 5.2 Error Handling Principles

**FR-SEARCH-ERROR-001:** SHALL catch all errors from searchApi.search()  
**FR-SEARCH-ERROR-002:** SHALL convert errors to user-friendly messages  
**FR-SEARCH-ERROR-003:** SHALL set error state (never throw)  
**FR-SEARCH-ERROR-004:** SHALL clear results on error  
**FR-SEARCH-ERROR-005:** SHALL clear metrics on error  
**FR-SEARCH-ERROR-006:** SHALL set isLoading to false on error  
**FR-SEARCH-ERROR-007:** SHALL NOT crash application on error  

---

## 6. Implementation Details

### 6.1 Hook Structure

```typescript
import { useState, useCallback } from 'react';
import { search } from '../services/searchApi';
import type { SearchResult, SearchResponse } from '../types/search';
import { validateQuery, validateLimit } from '../utils/validators';

interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number;
}

export function useSearch() {
  // State declarations
  const [query, setQueryState] = useState<string>('');
  const [limit, setLimitState] = useState<number>(10);
  const [results, setResults] = useState<SearchResult[]>([]);
  const [metrics, setMetrics] = useState<SearchMetrics | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(false);

  // setQuery implementation
  const setQuery = useCallback((newQuery: string) => {
    setQueryState(newQuery);
  }, []);

  // setLimit implementation
  const setLimit = useCallback((newLimit: number) => {
    if (validateLimit(newLimit)) {
      setLimitState(newLimit);
    }
  }, []);

  // executeSearch implementation
  const executeSearch = useCallback(async () => {
    const trimmedQuery = query.trim();
    if (!validateQuery(trimmedQuery)) {
      return; // Early return, no error set
    }

    setIsLoading(true);
    setError(null);
    setResults([]);
    const startTime = Date.now();

    try {
      const response: SearchResponse = await search(trimmedQuery, limit);
      
      const latency = Date.now() - startTime;
      setResults(response.results);
      setMetrics({
        query: response.query,
        resultCount: response.results.length,
        latency,
      });
    } catch (err) {
      // Convert error to user-friendly message
      let errorMessage = 'An error occurred';
      if (err instanceof Error) {
        errorMessage = err.message;
      }
      setError(errorMessage);
      setResults([]);
      setMetrics(null);
    } finally {
      setIsLoading(false);
    }
  }, [query, limit]);

  // clearSearch implementation
  const clearSearch = useCallback(() => {
    setQueryState('');
    setResults([]);
    setError(null);
    setMetrics(null);
  }, []);

  return {
    query,
    limit,
    results,
    metrics,
    error,
    isLoading,
    setQuery,
    setLimit,
    executeSearch,
    clearSearch,
  };
}
```

### 6.2 Dependencies

**External Dependencies:**
- `react` (useState, useCallback)
- `../services/searchApi` (search function)
- `../types/search` (SearchResult, SearchResponse types)
- `../utils/validators` (validateQuery, validateLimit)

**Internal Dependencies:**
- None (self-contained hook)

### 6.3 Performance Considerations

**Memoization:**
- Use `useCallback` for all returned functions
- Prevent unnecessary re-renders in consuming components
- Dependencies: `[query, limit]` for executeSearch

**State Updates:**
- Batch state updates where possible
- Use functional updates if needed
- Minimize re-renders

**Request Cancellation:**
- Current implementation does not cancel in-flight requests
- Future enhancement: Use AbortController for cancellation
- Results from cancelled requests are ignored if clearSearch() called

---

## 7. Test Requirements

### 7.1 Test Coverage Requirements

**TR-SEARCH-001:** Test initial state values  
**TR-SEARCH-002:** Test setQuery updates query state  
**TR-SEARCH-003:** Test setLimit validates and updates limit  
**TR-SEARCH-004:** Test setLimit rejects invalid values  
**TR-SEARCH-005:** Test executeSearch with valid query  
**TR-SEARCH-006:** Test executeSearch with empty query (no API call)  
**TR-SEARCH-007:** Test executeSearch with whitespace-only query  
**TR-SEARCH-008:** Test executeSearch updates results on success  
**TR-SEARCH-009:** Test executeSearch updates metrics on success  
**TR-SEARCH-010:** Test executeSearch calculates latency correctly  
**TR-SEARCH-011:** Test executeSearch handles NetworkError  
**TR-SEARCH-012:** Test executeSearch handles TimeoutError  
**TR-SEARCH-013:** Test executeSearch handles HttpError  
**TR-SEARCH-014:** Test executeSearch handles InvalidResponseError  
**TR-SEARCH-015:** Test executeSearch sets isLoading correctly  
**TR-SEARCH-016:** Test clearSearch resets all state  
**TR-SEARCH-017:** Test clearSearch does not reset limit  
**TR-SEARCH-018:** Test multiple rapid searches (latest wins)  

### 7.2 Test Structure

**Given-When-Then Format:**
- Given: Initial state or setup
- When: Action performed
- Then: Expected outcome

**Mock Requirements:**
- Mock `searchApi.search()` function
- Mock different error scenarios
- Mock successful responses

**Test Isolation:**
- Each test is independent
- Reset state between tests
- No shared state

---

## 8. Microservices Considerations

### 8.1 Stateless Design

**MS-SEARCH-001:** Hook maintains no server-side state  
**MS-SEARCH-002:** Each search is independent request  
**MS-SEARCH-003:** No session management required  
**MS-SEARCH-004:** Works with load-balanced backends  

### 8.2 API Communication

**MS-SEARCH-005:** Uses REST API only  
**MS-SEARCH-006:** No WebSocket or persistent connections  
**MS-SEARCH-007:** Each request includes all necessary data  
**MS-SEARCH-008:** No server-side state dependencies  

### 8.3 Scalability

**MS-SEARCH-009:** Hook can be used in multiple components  
**MS-SEARCH-010:** Multiple instances can run simultaneously  
**MS-SEARCH-011:** No coordination between instances needed  
**MS-SEARCH-012:** Supports horizontal scaling  

### 8.4 Failure Handling

**MS-SEARCH-013:** Graceful degradation when backend unavailable  
**MS-SEARCH-014:** Frontend remains functional  
**MS-SEARCH-015:** User-friendly error messages  
**MS-SEARCH-016:** No cascading failures  

---

## 9. Cursor Implementation Notes

When generating code from this specification:
- Use exact type signatures from Section 2.2 and 2.3
- Follow implementation structure from Section 6.1
- Include all error handling from Section 5
- Implement all test cases from Section 7
- Use useCallback for all returned functions
- Include JSDoc comments for the hook

---

## Appendices

### Appendix A: Usage Example

```typescript
import { useSearch } from '../hooks/useSearch';

function MyComponent() {
  const {
    query,
    limit,
    results,
    metrics,
    error,
    isLoading,
    setQuery,
    setLimit,
    executeSearch,
    clearSearch,
  } = useSearch();

  return (
    <div>
      <input value={query} onChange={(e) => setQuery(e.target.value)} />
      <button onClick={executeSearch} disabled={isLoading}>
        Search
      </button>
      {error && <div>Error: {error}</div>}
      {results.map((result) => (
        <div key={result.docId}>{result.snippet}</div>
      ))}
    </div>
  );
}
```

### Appendix B: Error Message Mapping

| Error Type | Error Message |
|------------|---------------|
| NetworkError | "Cannot connect to server" |
| TimeoutError | "Request timeout" |
| HttpError (500) | "Search failed with error 500" |
| InvalidResponseError | "Invalid server response" |

### Appendix C: Implementation Review Checklist

Before marking useSearch as complete:
- [ ] All FR-SEARCH requirements implemented (Section 4)
- [ ] All 18 test cases pass (Section 7)
- [ ] Zero TypeScript errors
- [ ] Code follows structure from Section 6.1
- [ ] Error handling matches Section 5
- [ ] All functions use useCallback
- [ ] JSDoc comments present

---

**END OF SPECIFICATION**

**Document ID:** SPEC-USE-SEARCH-001  
**Version:** 1.0  
**Status:** DRAFT  
**Next Document:** useHealthCheck_SPEC.md
