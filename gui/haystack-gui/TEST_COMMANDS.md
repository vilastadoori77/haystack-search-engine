# Test Commands Reference - Run Individual Tests

## Quick Tips to Reduce CPU Load:
1. Run tests one file at a time instead of all at once
2. Use `--run` flag to run once and exit (no watch mode)
3. Close other applications while testing
4. Run specific test cases instead of entire suites

---

## SearchBar Component Tests (15 tests)

```bash
# TC-001: Renders with empty input field
npm test -- -t "TC-001"

# TC-002: Submit button disabled when query is empty
npm test -- -t "TC-002"

# TC-003: Submit button enabled when query has text
npm test -- -t "TC-003"

# TC-004: Submit button disabled when loading
npm test -- -t "TC-004"

# TC-005: Calls onSearch when Enter key pressed
npm test -- -t "TC-005"

# TC-006: Does not call onSearch when Enter pressed with empty query
npm test -- -t "TC-006"

# TC-007: Calls onSearch when submit button clicked
npm test -- -t "TC-007"

# TC-008: Calls onQueryChange when input changes
npm test -- -t "TC-008"

# TC-009: Clear button visible when query has text
npm test -- -t "TC-009"

# TC-010: Clear button hidden when query is empty
npm test -- -t "TC-010"

# TC-011: Calls onClear when clear button clicked
npm test -- -t "TC-011"

# TC-012: Slider updates limit value
npm test -- -t "TC-012"

# TC-013: Slider displays current limit text
npm test -- -t "TC-013"

# TC-014: Slider respects min/max bounds
npm test -- -t "TC-014"

# TC-015: Input disabled when loading
npm test -- -t "TC-015"

# Run all SearchBar tests at once:
npm test -- SearchBar.test.tsx
```

---

## ResultsList Component Tests (8 tests)

```bash
# TC-016: Renders loading state when isLoading=true
npm test -- -t "TC-016"

# TC-017: Renders error state when error is not null
npm test -- -t "TC-017"

# TC-018: Renders empty state when no results and query exists
npm test -- -t "TC-018"

# TC-019: Does not render empty state when query is empty
npm test -- -t "TC-019"

# TC-020: Renders results when results array has items
npm test -- -t "TC-020"

# TC-021: Results list shows correct count
npm test -- -t "TC-021"

# TC-022: Loading state takes priority over results
npm test -- -t "TC-022"

# TC-023: Error state takes priority over results
npm test -- -t "TC-023"

# Run all ResultsList tests at once:
npm test -- ResultsList.test.tsx
```

---

## ResultCard Component Tests (5 tests)

```bash
# TC-024: Renders rank correctly
npm test -- -t "TC-024"

# TC-025: Renders document ID correctly
npm test -- -t "TC-025"

# TC-026: Renders score with 3 decimal places
npm test -- -t "TC-026"

# TC-027: Renders snippet text
npm test -- -t "TC-027"

# TC-028: Handles long snippets
npm test -- -t "TC-028"

# Run all ResultCard tests at once:
npm test -- ResultCard.test.tsx
```

---

## HealthStatus Component Tests (10 tests)

```bash
# TC-029: Renders green dot when healthy
npm test -- -t "TC-029"

# TC-030: Renders red dot when unhealthy
npm test -- -t "TC-030"

# TC-031: Renders gray dot when unknown
npm test -- -t "TC-031"

# TC-032: Displays "Healthy" text when healthy
npm test -- -t "TC-032"

# TC-033: Displays "Unavailable" text when unhealthy
npm test -- -t "TC-033"

# TC-034: Displays last checked timestamp
npm test -- -t "TC-034"

# TC-035: Displays "Never" when lastChecked is null
npm test -- -t "TC-035"

# TC-036: Calls onRefresh when refresh button clicked
npm test -- -t "TC-036"

# TC-037: Refresh button disabled when refreshing
npm test -- -t "TC-037"

# TC-038: Timestamp updates over time
npm test -- -t "TC-038"

# Run all HealthStatus tests at once:
npm test -- HealthStatus.test.tsx
```

---

## MetricsPanel Component Tests (6 tests)

```bash
# TC-039: Renders when metrics is not null
npm test -- -t "TC-039"

# TC-040: Does not render when metrics is null
npm test -- -t "TC-040"

# TC-041: Displays query text
npm test -- -t "TC-041"

# TC-042: Displays result count
npm test -- -t "TC-042"

# TC-043: Displays latency in milliseconds
npm test -- -t "TC-043"

# TC-044: Handles plural/singular for result count
npm test -- -t "TC-044"

# Run all MetricsPanel tests at once:
npm test -- MetricsPanel.test.tsx
```

---

## ExampleQueries Component Tests (4 tests)

```bash
# TC-045: Renders all four example query buttons
npm test -- -t "TC-045"

# TC-046: Calls onQuerySelect with correct query when button clicked
npm test -- -t "TC-046"

# TC-047: All buttons disabled when loading
npm test -- -t "TC-047"

# TC-048: All buttons enabled when not loading
npm test -- -t "TC-048"

# Run all ExampleQueries tests at once:
npm test -- ExampleQueries.test.tsx
```

---

## searchApi Service Tests (15 tests)

```bash
# TC-049: search() calls /search with encoded query string
npm test -- -t "TC-049"

# TC-050: search() includes k parameter in request
npm test -- -t "TC-050"

# TC-051: search() throws NetworkError on connection failure
npm test -- -t "TC-051"

# TC-052: search() throws TimeoutError on timeout
npm test -- -t "TC-052"

# TC-053: search() throws HttpError on 4xx/5xx response
npm test -- -t "TC-053"

# TC-054: search() throws InvalidResponseError on invalid JSON
npm test -- -t "TC-054"

# TC-055: search() throws InvalidResponseError on missing fields
npm test -- -t "TC-055"

# TC-056: search() validates query is non-empty
npm test -- -t "TC-056"

# TC-057: search() validates k is between 1 and 50
npm test -- -t "TC-057"

# TC-058: search() returns SearchResponse on success
npm test -- -t "TC-058"

# TC-059: checkHealth() returns healthy when server returns 200 OK
npm test -- -t "TC-059"

# TC-060: checkHealth() returns unhealthy when server returns 503
npm test -- -t "TC-060"

# TC-061: checkHealth() returns unhealthy on network error
npm test -- -t "TC-061"

# TC-062: checkHealth() returns unhealthy on timeout
npm test -- -t "TC-062"

# TC-063: checkHealth() uses shorter timeout than search
npm test -- -t "TC-063"

# Run all searchApi tests at once:
npm test -- searchApi.test.ts
```

---

## App Integration Tests (7 tests)

```bash
# TC-064: User can search and see results end-to-end
npm test -- -t "TC-064"

# TC-065: Error message displays when server unavailable
npm test -- -t "TC-065"

# TC-066: Example query auto-executes search
npm test -- -t "TC-066"

# TC-067: Health indicator updates automatically
npm test -- -t "TC-067"

# TC-068: Limit slider affects search results
npm test -- -t "TC-068"

# TC-069: Clear button resets state
npm test -- -t "TC-069"

# TC-070: Loading state shows during API call
npm test -- -t "TC-070"

# Run all App tests at once:
npm test -- App.test.tsx
```

---

## Formatters Utility Tests (3 tests)

```bash
# TC-071: formatScore() formats to 3 decimal places
npm test -- -t "TC-071"

# TC-072: formatTimestamp() formats relative time (seconds)
npm test -- -t "TC-072"

# TC-073: formatTimestamp() handles minutes
npm test -- -t "TC-073"

# Run all formatters tests at once:
npm test -- formatters.test.ts
```

---

## Validators Utility Tests (5 tests)

```bash
# TC-074: validateQuery() returns true for non-empty string
npm test -- -t "TC-074"

# TC-075: validateQuery() returns false for empty string
npm test -- -t "TC-075"

# TC-076: validateQuery() returns false for whitespace-only
npm test -- -t "TC-076"

# TC-077: validateLimit() returns true for 1-50
npm test -- -t "TC-077"

# TC-078: validateLimit() returns false for <1 or >50
npm test -- -t "TC-078"

# Run all validators tests at once:
npm test -- validators.test.ts
```

---

## Run Tests by Category (Less CPU Intensive)

### Run all component tests (one file at a time):
```bash
npm test -- SearchBar.test.tsx
npm test -- ResultsList.test.tsx
npm test -- ResultCard.test.tsx
npm test -- HealthStatus.test.tsx
npm test -- MetricsPanel.test.tsx
npm test -- ExampleQueries.test.tsx
npm test -- App.test.tsx
```

### Run all utility tests:
```bash
npm test -- formatters.test.ts
npm test -- validators.test.ts
```

### Run all service tests:
```bash
npm test -- searchApi.test.ts
```

---

## Performance Tips:

1. **Run with `--run` flag** (no watch mode):
   ```bash
   npm test -- --run -t "TC-001"
   ```

2. **Run one file at a time** instead of all tests:
   ```bash
   npm test -- --run SearchBar.test.tsx
   ```

3. **Limit concurrent workers** (add to vitest.config.ts):
   ```typescript
   test: {
     maxWorkers: 1,  // Run tests sequentially
     minWorkers: 1
   }
   ```

4. **Run tests in batches**:
   ```bash
   # Batch 1: Components
   npm test -- --run SearchBar ResultsList ResultCard
   
   # Batch 2: More Components
   npm test -- --run HealthStatus MetricsPanel ExampleQueries
   
   # Batch 3: Integration & Services
   npm test -- --run App searchApi
   
   # Batch 4: Utilities
   npm test -- --run formatters validators
   ```
