# Phase 2 GUI - 2: Write Test Files (TDD Red)

**Status:** IN PROGRESS  
**Phase:** Test-Driven Development (Red Phase)  
**Tool:** Cursor IDE  
**Duration:** 4-6 hours  
**Dependencies:** Phase 2 GUI - 1 (COMPLETE)

---

## 📋 Overview

Generate all 78+ test cases BEFORE writing implementation code. This is the **TDD Red Phase** - all tests should fail initially because no implementation exists yet.

---

## 🎯 Goals

- ✅ Write comprehensive test cases for all components and utilities
- ✅ Follow Given-When-Then (GWT) test structure
- ✅ Use Vitest + React Testing Library
- ✅ Mock API calls with axios-mock-adapter
- ✅ Achieve >90% test coverage specification

---

## 📊 Task Breakdown

| Task | File | Test Cases | Count | Status |
|------|------|------------|-------|--------|
| 1 | searchApi.test.ts | TC-049 to TC-063 | 15 | ⏳ NEXT |
| 2 | formatters.test.ts | TC-071 to TC-073 | 3 | ⏸️ TODO |
| 3 | validators.test.ts | TC-074 to TC-078 | 5 | ⏸️ TODO |
| 4 | SearchBar.test.tsx | TC-001 to TC-015 | 15 | ⏸️ TODO |
| 5 | ResultsList.test.tsx | TC-016 to TC-023 | 8 | ⏸️ TODO |
| 6 | ResultCard.test.tsx | TC-024 to TC-028 | 5 | ⏸️ TODO |
| 7 | HealthStatus.test.tsx | TC-029 to TC-038 | 10 | ⏸️ TODO |
| 8 | MetricsPanel.test.tsx | TC-039 to TC-044 | 6 | ⏸️ TODO |
| 9 | ExampleQueries.test.tsx | TC-045 to TC-048 | 4 | ⏸️ TODO |
| 10 | App.test.tsx | TC-064 to TC-070 | 7 | ⏸️ TODO |

**Total:** 78 test cases

---

## 🚀 Getting Started

### Prerequisites

1. ✅ Phase 2 GUI - 1 completed
2. ✅ Cursor IDE installed
3. ✅ Project open in Cursor: `/Users/vilastadoori/_Haystack_proj/gui/haystack-gui`

### Initial Setup Commands

Before generating tests, create placeholder files:

```bash
cd /Users/vilastadoori/_Haystack_proj/gui/haystack-gui

# Create type definitions
touch src/types/search.ts

# Create API client
touch src/services/searchApi.ts

# Create utility files  
touch src/utils/formatters.ts
touch src/utils/validators.ts
```

---

## 📝 Task 1: Generate searchApi.test.ts

### Test Cases: TC-049 through TC-063 (15 tests)

### Cursor Prompt

**Open Cursor Chat (Cmd + L) and paste this:**

```
I'm implementing Phase 2 GUI for a search engine frontend. I need to generate test file for the API client following Test-Driven Development (TDD).

Generate the test file: src/services/searchApi.test.ts

Requirements from specification:
- Use Vitest as test runner with describe/it/expect syntax
- Use axios-mock-adapter for mocking HTTP calls
- Test the search() function with test cases TC-049 through TC-063
- Test the checkHealth() function

Test Cases to Implement:

TC-049: search() calls /search with encoded query string
- Given: API client configured
- When: search("migration validation", 10) called
- Then: GET request to /search?q=migration%20validation&k=10

TC-050: search() includes k parameter in request
- Given: API client configured
- When: search("test", 25) called
- Then: Request includes k=25 parameter

TC-051: search() throws NetworkError on connection failure
- Given: Server not running
- When: search("test", 10) called
- Then: NetworkError thrown with message "Cannot connect to server..."

TC-052: search() throws TimeoutError on timeout
- Given: Server delays >5s
- When: search("test", 10) called
- Then: TimeoutError thrown

TC-053: search() throws HttpError on 4xx/5xx response
- Given: Server returns 500
- When: search("test", 10) called
- Then: HttpError thrown with status 500

TC-054: search() throws InvalidResponseError on invalid JSON
- Given: Server returns non-JSON
- When: search("test", 10) called
- Then: InvalidResponseError thrown

TC-055: search() throws InvalidResponseError on missing fields
- Given: Server returns JSON without results field
- When: search("test", 10) called
- Then: InvalidResponseError thrown

TC-056: search() validates query is non-empty
- Given: API client configured
- When: search("", 10) called
- Then: Error thrown before API call

TC-057: search() validates k is between 1 and 50
- Given: API client configured
- When: search("test", 0) or search("test", 51) called
- Then: Error thrown before API call

TC-058: search() returns SearchResponse on success
- Given: Server returns valid response
- When: search("test", 10) called
- Then: SearchResponse object returned

TC-059: checkHealth() returns healthy when server returns 200 OK
- Given: Server returns 200 with "OK" body
- When: checkHealth() called
- Then: Returns {status: 'healthy'}

TC-060: checkHealth() returns unhealthy when server returns 503
- Given: Server returns 503
- When: checkHealth() called
- Then: Returns {status: 'unhealthy'}

TC-061: checkHealth() returns unhealthy on network error
- Given: Server not running
- When: checkHealth() called
- Then: Returns {status: 'unhealthy'} (does not throw)

TC-062: checkHealth() returns unhealthy on timeout
- Given: Server delays >2s
- When: checkHealth() called
- Then: Returns {status: 'unhealthy'}

TC-063: checkHealth() uses shorter timeout than search
- Given: API client configured
- When: checkHealth() called
- Then: Timeout is 2000ms (not 5000ms)

API Specification:
- Base URL: http://localhost:8900
- Search endpoint: GET /search?q={query}&k={limit}
- Health endpoint: GET /health
- Search timeout: 5000ms
- Health timeout: 2000ms

Error types to import from src/types/search.ts:
- NetworkError
- TimeoutError  
- HttpError
- InvalidResponseError

Response types from src/types/search.ts:
- SearchResponse: { query: string, results: SearchResult[] }
- SearchResult: { docId: number, score: number, snippet: string }
- HealthCheckResponse: { status: HealthStatus }
- HealthStatus: 'healthy' | 'unhealthy' | 'unknown'

Generate complete test file with:
1. All necessary imports
2. Mock setup with axios-mock-adapter
3. All 15 test cases (TC-049 to TC-063)
4. Proper Given-When-Then structure
5. Clean mock reset between tests
```

### Expected Output

Cursor should generate a file with:
- Import statements (vitest, axios-mock-adapter, types)
- Mock setup with beforeEach/afterEach
- describe('search') block with 10 tests
- describe('checkHealth') block with 5 tests
- Proper assertions and error checking

### Verification

After generation, run:

```bash
npm test src/services/searchApi.test.ts
```

**Expected Result:** All tests should FAIL (Red phase) because `searchApi.ts` is empty.

---

## 📝 Task 2: Generate formatters.test.ts

### Test Cases: TC-071 through TC-073 (3 tests)

### Cursor Prompt

```
Generate test file: src/utils/formatters.test.ts

Requirements:
- Use Vitest
- Test formatScore() and formatTimestamp() functions

Test Cases:

TC-071: formatScore() formats to 3 decimal places
- Given: Score = 2.456789
- When: formatScore(score) called
- Then: Returns "2.457"

TC-072: formatTimestamp() formats relative time
- Given: Date 5 seconds ago
- When: formatTimestamp(date) called
- Then: Returns "5s ago"

TC-073: formatTimestamp() handles minutes
- Given: Date 2 minutes ago
- When: formatTimestamp(date) called
- Then: Returns "2m ago"

Function signatures:
- formatScore(score: number): string
- formatTimestamp(date: Date): string

Additional test cases to add:
- Test formatTimestamp with hours (e.g., "2h ago")
- Test formatTimestamp with days (e.g., "3d ago")
- Test formatScore with edge cases (0, negative, very large numbers)
- Test formatTimestamp with null/undefined (should handle gracefully)

Generate complete test file with proper Vitest syntax.
```

### Verification

```bash
npm test src/utils/formatters.test.ts
```

**Expected:** All tests FAIL (formatters.ts is empty)

---

## 📝 Task 3: Generate validators.test.ts

### Test Cases: TC-074 through TC-078 (5 tests)

### Cursor Prompt

```
Generate test file: src/utils/validators.test.ts

Requirements:
- Use Vitest
- Test validateQuery() and validateLimit() functions

Test Cases:

TC-074: validateQuery() returns true for non-empty string
- Given: Query = "test"
- When: validateQuery(query) called
- Then: Returns true

TC-075: validateQuery() returns false for empty string
- Given: Query = ""
- When: validateQuery(query) called
- Then: Returns false

TC-076: validateQuery() returns false for whitespace-only
- Given: Query = "   "
- When: validateQuery(query) called
- Then: Returns false

TC-077: validateLimit() returns true for 1-50
- Given: Limit = 25
- When: validateLimit(limit) called
- Then: Returns true

TC-078: validateLimit() returns false for <1 or >50
- Given: Limit = 0 or 51
- When: validateLimit(limit) called
- Then: Returns false

Function signatures:
- validateQuery(query: string): boolean
- validateLimit(limit: number): boolean

Additional edge cases:
- Test validateQuery with special characters
- Test validateLimit with decimal numbers
- Test validateLimit with negative numbers
- Test validators with null/undefined inputs

Generate complete test file with proper Vitest syntax.
```

### Verification

```bash
npm test src/utils/validators.test.ts
```

**Expected:** All tests FAIL

---

## 📝 Task 4: Generate SearchBar.test.tsx

### Test Cases: TC-001 through TC-015 (15 tests)

### Cursor Prompt

```
Generate test file: src/components/SearchBar.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test SearchBar component (controlled component)
- Use @testing-library/user-event for interactions

Props interface:
interface SearchBarProps {
  query: string;
  limit: number;
  isLoading: boolean;
  onQueryChange: (query: string) => void;
  onLimitChange: (limit: number) => void;
  onSearch: () => void;
  onClear: () => void;
}

Test Cases:

TC-001: Renders with empty input field
TC-002: Submit button disabled when query is empty
TC-003: Submit button enabled when query has text
TC-004: Submit button disabled when loading
TC-005: Calls onSearch when Enter key pressed
TC-006: Does not call onSearch when Enter pressed with empty query
TC-007: Calls onSearch when submit button clicked
TC-008: Calls onQueryChange when input changes
TC-009: Clear button visible when query has text
TC-010: Clear button hidden when query is empty
TC-011: Calls onClear when clear button clicked
TC-012: Slider updates limit value
TC-013: Slider displays current limit
TC-014: Slider respects min/max bounds (1-50)
TC-015: Input disabled when loading

Component features:
- Query input with placeholder "Enter your search query..."
- Submit button (disabled when empty or loading)
- Clear button (visible only when query has text)
- Result limit slider (range 1-50, displays "Limit: {value}")
- Enter key triggers search (only if query not empty)

Generate complete test file with:
1. Proper imports
2. Mock functions (vi.fn()) for callbacks
3. All 15 test cases
4. Proper cleanup between tests
5. User interaction testing (typing, clicking, key presses)
```

### Verification

```bash
npm test src/components/SearchBar.test.tsx
```

**Expected:** All tests FAIL (SearchBar.tsx doesn't exist)

---

## 📝 Task 5: Generate ResultsList.test.tsx

### Test Cases: TC-016 through TC-023 (8 tests)

### Cursor Prompt

```
Generate test file: src/components/ResultsList.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test ResultsList component (presentation component)

Props interface:
interface ResultsListProps {
  results: SearchResult[];
  isLoading: boolean;
  error: string | null;
  query: string;
}

interface SearchResult {
  docId: number;
  score: number;
  snippet: string;
}

Test Cases:

TC-016: Renders loading state when isLoading=true
TC-017: Renders error state when error is not null
TC-018: Renders empty state when no results and query exists
TC-019: Does not render empty state when query is empty
TC-020: Renders results when results array has items
TC-021: Results list shows correct count
TC-022: Loading state takes priority over results
TC-023: Error state takes priority over results

State priority (most important first):
1. Loading (if isLoading)
2. Error (if error !== null)
3. Empty (if no results and query exists)
4. Results (if results.length > 0)

Component states:
- Loading: Shows spinner and "Searching..." text
- Error: Shows error message in red
- Empty: Shows "No results found. Try a different query."
- Success: Renders ResultCard for each result

Generate complete test file with proper state priority testing.
```

### Verification

```bash
npm test src/components/ResultsList.test.tsx
```

---

## 📝 Task 6: Generate ResultCard.test.tsx

### Test Cases: TC-024 through TC-028 (5 tests)

### Cursor Prompt

```
Generate test file: src/components/ResultCard.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test ResultCard component (simple presentation component)

Props interface:
interface ResultCardProps {
  result: SearchResult;
  rank: number;
}

interface SearchResult {
  docId: number;
  score: number;
  snippet: string;
}

Test Cases:

TC-024: Renders rank correctly (e.g., "#1")
TC-025: Renders document ID correctly (e.g., "Document 5")
TC-026: Renders score with 3 decimal places (e.g., "2.457")
TC-027: Renders snippet text
TC-028: Handles long snippets (truncation or wrapping)

Display format:
- Rank: "#{rank}"
- Document ID: "Document {docId}"
- Score: formatted to 3 decimal places
- Snippet: full text displayed

Accessibility:
- Card has role="article"
- Card has aria-label="Search result {rank}"
- Score has aria-label="Relevance score {score}"

Generate complete test file with accessibility checks.
```

---

## 📝 Task 7: Generate HealthStatus.test.tsx

### Test Cases: TC-029 through TC-038 (10 tests)

### Cursor Prompt

```
Generate test file: src/components/HealthStatus.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test HealthStatus component
- Mock timers for timestamp updates

Props interface:
interface HealthStatusProps {
  health: HealthStatus;
  lastChecked: Date | null;
  onRefresh: () => void;
  isRefreshing: boolean;
}

type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';

Test Cases:

TC-029: Renders green dot when healthy
TC-030: Renders red dot when unhealthy
TC-031: Renders gray dot when unknown
TC-032: Displays "Healthy" text when healthy
TC-033: Displays "Unavailable" text when unhealthy
TC-034: Displays last checked timestamp (e.g., "2s ago")
TC-035: Displays "Never" when lastChecked is null
TC-036: Calls onRefresh when refresh button clicked
TC-037: Refresh button disabled when refreshing
TC-038: Timestamp updates every second (use fake timers)

Component features:
- Health indicator (colored dot)
- Status text ("Healthy"/"Unavailable"/"Unknown")
- Relative timestamp ("2s ago", "1m ago", etc.)
- Refresh button (disabled when refreshing)

For TC-038, use vi.useFakeTimers() to test timestamp updates.

Generate complete test file with timer mocking.
```

---

## 📝 Task 8: Generate MetricsPanel.test.tsx

### Test Cases: TC-039 through TC-044 (6 tests)

### Cursor Prompt

```
Generate test file: src/components/MetricsPanel.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test MetricsPanel component (conditional rendering)

Props interface:
interface MetricsPanelProps {
  metrics: SearchMetrics | null;
}

interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number; // milliseconds
}

Test Cases:

TC-039: Renders when metrics is not null
TC-040: Does not render when metrics is null
TC-041: Displays query text (e.g., "Query: migration")
TC-042: Displays result count (e.g., "3 result(s) found")
TC-043: Displays latency in milliseconds (e.g., "Latency: 123ms")
TC-044: Handles plural/singular for result count (1 result vs 3 results)

Display format:
- Query: "Query: {query}"
- Count: "{count} result(s) found" or "{count} result found" (singular)
- Latency: "Latency: {latency}ms"

Component only renders when metrics !== null.

Accessibility:
- Panel has role="region"
- Panel has aria-label="Search metrics"

Generate complete test file with null handling tests.
```

---

## 📝 Task 9: Generate ExampleQueries.test.tsx

### Test Cases: TC-045 through TC-048 (4 tests)

### Cursor Prompt

```
Generate test file: src/components/ExampleQueries.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Test ExampleQueries component

Props interface:
interface ExampleQueriesProps {
  onQuerySelect: (query: string) => void;
  isLoading: boolean;
}

Test Cases:

TC-045: Renders all four example query buttons
TC-046: Calls onQuerySelect with correct query when button clicked
TC-047: Buttons disabled when loading
TC-048: Buttons enabled when not loading

Example queries (4 buttons):
1. "migration" (single term)
2. "migration validation" (implicit AND)
3. "migration OR validation" (explicit OR)
4. "schema -migration" (NOT query)

Component features:
- Four pre-defined query buttons
- Each button calls onQuerySelect with the query string
- All buttons disabled when isLoading === true

Accessibility:
- Each button has aria-label="Search for: {query}"
- Container has role="group" and aria-label="Example queries"

Generate complete test file with button interaction tests.
```

---

## 📝 Task 10: Generate App.test.tsx

### Test Cases: TC-064 through TC-070 (7 integration tests)

### Cursor Prompt

```
Generate test file: src/App.test.tsx

Requirements:
- Use Vitest + React Testing Library
- Integration tests for full App component
- Mock API calls with axios-mock-adapter
- Mock timers for health check polling

Test Cases:

TC-064: User can search and see results end-to-end
- Given: Server running, app loaded
- When: User enters query, clicks Search
- Then: Results displayed, metrics shown, no errors

TC-065: Error message displays when server unavailable
- Given: Server not running
- When: User enters query, clicks Search
- Then: Error message displayed in ResultsList

TC-066: Example query auto-executes search
- Given: Server running
- When: User clicks example query button
- Then: Query set, search triggered, results displayed

TC-067: Health indicator updates automatically
- Given: App loaded, server running
- When: Server stops (mock network failure)
- Then: Health indicator updates to unhealthy within 5 seconds

TC-068: Limit slider affects search results
- Given: Server running
- When: User sets limit to 25, searches
- Then: API called with k=25

TC-069: Clear button resets state
- Given: Search performed, results shown
- When: User clicks Clear
- Then: Query empty, results cleared, metrics cleared

TC-070: Loading state shows during API call
- Given: Server running
- When: User searches (with delayed response)
- Then: Loading spinner visible during request

App state management:
- query, limit, results, metrics, error, isLoading
- health, healthLastChecked, isHealthRefreshing

These are integration tests - they test the full user flow through the app.

Generate complete test file with:
1. API mocking setup
2. Timer mocking for health checks
3. User interaction simulation
4. Full state verification
```

---

## ✅ Verification Checklist

After generating all test files:

### Run All Tests

```bash
npm test
```

**Expected Result:** All tests should FAIL (Red phase)

This is CORRECT for TDD! Tests fail because no implementation exists yet.

### Check Test Count

```bash
npm test -- --reporter=verbose
```

**Expected:** Should see 78+ test cases listed

### Check Coverage

```bash
npm test -- --coverage
```

**Expected:** 0% coverage (no implementation yet)

---

## 📊 Success Criteria

- [ ] All 10 test files created
- [ ] 78+ test cases generated
- [ ] All tests use proper Vitest syntax
- [ ] All tests follow Given-When-Then structure
- [ ] All tests currently FAIL (Red phase)
- [ ] No syntax errors in test files
- [ ] Imports are correct (even if modules don't exist yet)

---

## 🔍 Common Issues and Solutions

### Issue 1: Import Errors

**Problem:** Tests show import errors for components/services that don't exist

**Solution:** This is expected! We're in TDD Red phase. These will be fixed in Phase 2 GUI - 3.

### Issue 2: Cursor Generates Wrong Syntax

**Problem:** Cursor uses Jest syntax instead of Vitest

**Solution:** In prompt, emphasize "Use Vitest (NOT Jest)" and give examples:
```javascript
// Vitest (correct)
import { describe, it, expect, vi } from 'vitest'

// NOT Jest
import { describe, it, expect, jest } from '@jest/globals'
```

### Issue 3: Tests Don't Follow Spec

**Problem:** Generated tests don't match TC-XXX specifications

**Solution:** Re-prompt Cursor with specific test case details from spec

### Issue 4: Mock Setup Incorrect

**Problem:** axios-mock-adapter not configured properly

**Solution:** Provide example:
```javascript
import MockAdapter from 'axios-mock-adapter';
import axios from 'axios';

const mock = new MockAdapter(axios);

beforeEach(() => {
  mock.reset();
});
```

---

## 📝 Tips for Working with Cursor

### Best Practices

1. **One file at a time** - Don't try to generate multiple files in one prompt
2. **Be specific** - Reference test case numbers (TC-XXX)
3. **Provide context** - Include interfaces and types in prompt
4. **Review output** - Always check generated code matches spec
5. **Iterate if needed** - Re-prompt if output is wrong

### Prompt Structure Template

```
Generate test file: [filepath]

Requirements:
- [Testing framework]
- [Component/function being tested]

Props/Function Signatures:
[Interfaces/types]

Test Cases:
TC-XXX: [Description]
- Given: [Precondition]
- When: [Action]
- Then: [Expected result]

[Repeat for each test case]

Additional Requirements:
- [Any special setup]
- [Mock configuration]
- [Edge cases]

Generate complete test file with [specific features].
```

---

## 🎯 Next Steps

After completing Phase 2 GUI - 2:

**Phase 2 GUI - 3: Implement API Client (TDD Green)**
- Create `src/types/search.ts` with all interfaces
- Implement `src/services/searchApi.ts`
- Make API tests pass (Green phase)

**Phase 2 GUI - 4: Implement Utilities**
- Implement `formatters.ts`
- Implement `validators.ts`
- Make utility tests pass

**Phase 2 GUI - 5: Implement Components**
- Implement all React components
- Make all component tests pass
- Build complete UI

---

## 📚 Reference Links

- **Vitest Docs:** https://vitest.dev/
- **React Testing Library:** https://testing-library.com/react
- **axios-mock-adapter:** https://github.com/ctimmerm/axios-mock-adapter
- **Phase 2 GUI Spec:** See `PHASE_2_GUI_SPEC.md`

---

## 📊 Progress Tracking

Update this as you complete tasks:

- [ ] Task 1: searchApi.test.ts
- [ ] Task 2: formatters.test.ts
- [ ] Task 3: validators.test.ts
- [ ] Task 4: SearchBar.test.tsx
- [ ] Task 5: ResultsList.test.tsx
- [ ] Task 6: ResultCard.test.tsx
- [ ] Task 7: HealthStatus.test.tsx
- [ ] Task 8: MetricsPanel.test.tsx
- [ ] Task 9: ExampleQueries.test.tsx
- [ ] Task 10: App.test.tsx

**Total Test Cases:** 0 / 78

---

**Generated:** January 25, 2026  
**Project:** Haystack Search Engine Phase 2 GUI  
**Developer:** Vilas Tadoori  
**Tool:** Cursor IDE for test generation
