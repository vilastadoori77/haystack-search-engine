# Phase 2 GUI Specification

## Spec Status

**Status:** ✅ Approved  
**Version:** 1.0  
**Date:** 2026-01-XX  
**Dependencies:** Phase 2 Backend (COMPLETE)  
**Target Technology:** React 18+, TypeScript, Vite, Axios, Tailwind CSS

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Component Specifications](#3-component-specifications)
4. [API Client Specification](#4-api-client-specification)
5. [TypeScript Type Definitions](#5-typescript-type-definitions)
6. [User Flows](#6-user-flows)
7. [Error Handling Strategy](#7-error-handling-strategy)
8. [Test Plan](#8-test-plan)
9. [UI Mockups](#9-ui-mockups)
10. [Development Plan](#10-development-plan)
11. [Acceptance Criteria](#11-acceptance-criteria)
12. [Open Questions / Assumptions](#12-open-questions--assumptions)

---

## 1. Executive Summary

### 1.1 Purpose

The Phase 2 GUI is a React TypeScript web application designed to validate and demonstrate the Phase 2 backend capabilities of the Haystack Search Engine. It provides an intuitive interface for executing search queries, viewing results, monitoring server health, and exploring example queries.

### 1.2 Scope

#### Included
- Search query input and execution
- Search results display with ranking, scores, and snippets
- Server health monitoring with auto-refresh
- Query metrics (latency, result count)
- Example query buttons for common search patterns
- Error handling and user feedback
- Responsive design for desktop and mobile

#### Excluded
- User authentication or authorization
- Search history or saved queries
- Advanced filtering or faceting
- Result pagination (backend handles via `k` parameter)
- Multi-language support
- Dark mode or theme switching
- Export or sharing functionality

### 1.3 Target Users

- **Primary**: Developers and QA engineers validating Phase 2 backend functionality
- **Secondary**: Product managers and stakeholders reviewing search capabilities
- **Tertiary**: End users testing search functionality before production deployment

### 1.4 Success Criteria

The Phase 2 GUI is considered successful when:

1. All functional requirements (FR-1 through FR-6) are implemented and working
2. All non-functional requirements (NFR-1 through NFR-7) are met
3. Test coverage exceeds 80% for components and 90% for API client
4. Application loads in <2 seconds on standard hardware
5. All acceptance criteria are verified and documented
6. Code passes TypeScript strict mode compilation with zero `any` types
7. Application is accessible via keyboard navigation and screen readers

### 1.5 Development Approach

**Test-Driven Development (TDD)** is mandatory:

1. **Red**: Write failing test cases first
2. **Green**: Implement minimal code to pass tests
3. **Refactor**: Improve code quality while maintaining green tests

All test cases must be written BEFORE implementation code.

---

## 2. Architecture Overview

### 2.1 Component Hierarchy

```
App (Root Component)
├── Header
│   ├── Title: "Haystack Search Engine"
│   └── HealthStatus
│       ├── HealthIndicator (green/red dot)
│       ├── StatusText ("Healthy" / "Unavailable")
│       ├── LastCheckedTimestamp
│       └── RefreshButton
├── Main Content Area
│   ├── SearchSection
│   │   ├── SearchBar
│   │   │   ├── QueryInput
│   │   │   ├── SubmitButton
│   │   │   └── ClearButton
│   │   ├── ResultLimitSlider
│   │   │   ├── Slider (1-50)
│   │   │   └── LimitDisplay ("Limit: 10")
│   │   └── ExampleQueries
│   │       └── QueryButton[] (4 buttons)
│   ├── MetricsPanel
│   │   ├── QueryText
│   │   ├── ResultCount
│   │   └── Latency
│   └── ResultsList
│       ├── LoadingState (spinner + "Searching...")
│       ├── EmptyState ("No results found...")
│       ├── ErrorState (error message)
│       └── ResultCard[] (rank, docId, score, snippet)
└── Footer
    └── Copyright / Version Info
```

### 2.2 Data Flow

```
User Action
    ↓
Component Event Handler
    ↓
API Client (searchApi.ts)
    ↓
HTTP Request (Axios)
    ↓
Backend Server (searchd on :8900)
    ↓
HTTP Response
    ↓
API Client (transform/validate)
    ↓
Component State Update (useState/useEffect)
    ↓
UI Re-render (React)
```

### 2.3 State Management Strategy

**React Hooks Only** (no Redux, Zustand, or Context API for global state):

- **Local Component State**: `useState` for component-specific state
- **API State**: Custom hooks (`useSearch`, `useHealthCheck`) encapsulate API calls and state
- **Derived State**: Computed values from props/state using `useMemo`
- **Side Effects**: `useEffect` for API calls, timers, cleanup

**State Locations:**
- `App.tsx`: Global search state (query, results, metrics, error)
- `SearchBar.tsx`: Input field value, validation state
- `HealthStatus.tsx`: Health status, last checked timestamp
- `ResultsList.tsx`: Loading state (derived from App state)

### 2.4 Folder Structure

```
src/
├── components/
│   ├── SearchBar.tsx          # Query input, submit, clear
│   ├── ResultsList.tsx         # Results container with states
│   ├── ResultCard.tsx          # Individual result display
│   ├── HealthStatus.tsx       # Health indicator and refresh
│   ├── MetricsPanel.tsx       # Query metrics display
│   ├── ExampleQueries.tsx     # Pre-defined query buttons
│   └── LoadingSpinner.tsx      # Reusable loading indicator
├── services/
│   └── searchApi.ts           # Axios-based API client
├── types/
│   └── search.ts              # TypeScript interfaces
├── hooks/
│   ├── useSearch.ts           # Search query hook
│   ├── useHealthCheck.ts      # Health monitoring hook
│   └── useDebounce.ts        # Debounce utility hook
├── utils/
│   ├── formatters.ts          # Score, timestamp formatting
│   └── validators.ts          # Input validation
├── App.tsx                    # Main orchestrator component
└── main.tsx                   # Entry point (Vite)
```

### 2.5 Technology Stack

| Technology | Version | Purpose |
|------------|---------|---------|
| React | 18+ | UI framework |
| TypeScript | 5+ | Type safety |
| Vite | 5+ | Build tool and dev server |
| Axios | 1.6+ | HTTP client |
| Tailwind CSS | 3+ | Styling |
| Vitest | 1+ | Test runner |
| React Testing Library | 14+ | Component testing |

---

## 3. Component Specifications

### 3.1 SearchBar Component

#### Purpose
Provides the primary search interface with query input, submit button, clear button, and result limit slider.

#### Props Interface

```typescript
interface SearchBarProps {
  query: string;
  limit: number;
  isLoading: boolean;
  onQueryChange: (query: string) => void;
  onLimitChange: (limit: number) => void;
  onSearch: () => void;
  onClear: () => void;
}
```

#### State
- None (controlled component, all state from props)

#### Behavior

1. **Query Input**:
   - Controlled input field bound to `query` prop
   - Updates via `onQueryChange` on every keystroke
   - Placeholder: "Enter your search query..."
   - Auto-focus on mount (optional, for UX)

2. **Submit Button**:
   - Disabled when `query.trim().length === 0`
   - Disabled when `isLoading === true`
   - Text: "Search" (or "Searching..." when loading)
   - Calls `onSearch()` on click

3. **Enter Key**:
   - Pressing Enter in input field calls `onSearch()`
   - Only if query is not empty and not loading

4. **Clear Button**:
   - Visible only when `query.length > 0`
   - Calls `onClear()` on click
   - Resets query to empty string (via parent state)

5. **Result Limit Slider**:
   - Range: 1 to 50
   - Default: 10
   - Updates via `onLimitChange` on slider change
   - Displays current value: "Limit: {limit}"

#### Rendering Logic

**States:**
- **Default**: Input empty, submit disabled, clear hidden
- **With Query**: Input has text, submit enabled, clear visible
- **Loading**: Submit shows "Searching...", submit disabled, input disabled

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  Search Query                                       │
│  ┌─────────────────────────────────────────────┐  │
│  │ Enter your search query...              [X] │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
│  Result Limit: [━━━━━━━━●━━━━━━━━] 10             │
│                                                     │
│  [Search]                    [Clear]               │
└─────────────────────────────────────────────────────┘
```

#### Accessibility

- Input has `aria-label="Search query input"`
- Submit button has `aria-label="Submit search"`
- Clear button has `aria-label="Clear search query"`
- Slider has `aria-label="Result limit"` and `aria-valuemin="1"`, `aria-valuemax="50"`

---

### 3.2 ResultsList Component

#### Purpose
Displays search results in a list format, handling loading, empty, error, and success states.

#### Props Interface

```typescript
interface ResultsListProps {
  results: SearchResult[];
  isLoading: boolean;
  error: string | null;
  query: string;
}
```

#### State
- None (presentation component)

#### Behavior

1. **Loading State**:
   - Shows when `isLoading === true`
   - Displays spinner and "Searching..." text
   - Hides results

2. **Error State**:
   - Shows when `error !== null`
   - Displays error message in red
   - Hides results

3. **Empty State**:
   - Shows when `results.length === 0` and not loading and no error
   - Only if `query.length > 0` (user has searched)
   - Message: "No results found. Try a different query."

4. **Success State**:
   - Shows when `results.length > 0`
   - Renders `ResultCard` for each result
   - Results already sorted by score (backend handles this)

#### Rendering Logic

**State Priority:**
1. Loading (if `isLoading`)
2. Error (if `error`)
3. Empty (if no results and query exists)
4. Results (if results exist)

#### Visual Example (ASCII)

**Loading:**
```
┌─────────────────────────────────────────────────────┐
│  Search Results                                     │
│                                                     │
│         ⏳                                          │
│    Searching...                                     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**Empty:**
```
┌─────────────────────────────────────────────────────┐
│  Search Results                                     │
│                                                     │
│  No results found. Try a different query.           │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**Success:**
```
┌─────────────────────────────────────────────────────┐
│  Search Results (3 found)                           │
│                                                     │
│  [ResultCard #1]                                    │
│  [ResultCard #2]                                    │
│  [ResultCard #3]                                    │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

### 3.3 ResultCard Component

#### Purpose
Displays a single search result with rank, document ID, score, and snippet.

#### Props Interface

```typescript
interface ResultCardProps {
  result: SearchResult;
  rank: number;
}
```

Where `SearchResult` is:
```typescript
interface SearchResult {
  docId: number;
  score: number;
  snippet: string;
}
```

#### State
- None (presentation component)

#### Behavior

1. **Display Format**:
   - Rank: "#{rank}" (e.g., "#1", "#2")
   - Document ID: "Document {docId}"
   - Score: "{score}" formatted to 3 decimal places (e.g., "2.456")
   - Snippet: Full snippet text, with query terms highlighted (optional enhancement)

2. **Layout**:
   - Card-based design with border and padding
   - Rank and docId on same line
   - Score on separate line or inline
   - Snippet as paragraph text

#### Rendering Logic

- Always renders if `result` prop is provided
- No conditional rendering (parent handles empty state)

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  #1  Document 1                    Score: 2.456    │
│  ─────────────────────────────────────────────────  │
│  Teamcenter migration guide: map attributes,       │
│  validate schema, run dry-run.                     │
└─────────────────────────────────────────────────────┘
```

#### Accessibility

- Card has `role="article"` and `aria-label="Search result {rank}"`
- Score has `aria-label="Relevance score {score}"`

---

### 3.4 HealthStatus Component

#### Purpose
Displays server health status with visual indicator, auto-refresh, and manual refresh capability.

#### Props Interface

```typescript
interface HealthStatusProps {
  health: HealthStatus;
  lastChecked: Date | null;
  onRefresh: () => void;
  isRefreshing: boolean;
}
```

Where `HealthStatus` is:
```typescript
type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';
```

#### State
- None (controlled component)

#### Behavior

1. **Health Indicator**:
   - Green dot (●) when `health === 'healthy'`
   - Red dot (●) when `health === 'unhealthy'`
   - Gray dot (○) when `health === 'unknown'`

2. **Status Text**:
   - "Healthy" when `health === 'healthy'`
   - "Unavailable" when `health === 'unhealthy'`
   - "Unknown" when `health === 'unknown'`

3. **Last Checked Timestamp**:
   - Shows relative time: "2s ago", "1m ago", etc.
   - Updates every second (client-side timer)
   - Shows "Never" if `lastChecked === null`

4. **Refresh Button**:
   - Always visible
   - Disabled when `isRefreshing === true`
   - Text: "Refresh" (or "Refreshing..." when refreshing)
   - Calls `onRefresh()` on click

5. **Auto-Refresh**:
   - Handled by parent component (App.tsx)
   - Polls `/health` every 5 seconds
   - Stops polling when component unmounts

#### Rendering Logic

**States:**
- **Healthy**: Green dot, "Healthy", timestamp, refresh button
- **Unhealthy**: Red dot, "Unavailable", timestamp, refresh button
- **Unknown**: Gray dot, "Unknown", "Never", refresh button
- **Refreshing**: All states, refresh button disabled

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  Server Health                                      │
│                                                     │
│  ● Healthy                    Last checked: 2s ago │
│                                                     │
│  [Refresh]                                          │
└─────────────────────────────────────────────────────┘
```

#### Accessibility

- Health indicator has `aria-label="Server health: {status}"`
- Refresh button has `aria-label="Refresh health status"`
- Status text has `aria-live="polite"` for screen reader updates

---

### 3.5 MetricsPanel Component

#### Purpose
Displays query metrics after a search: query text, result count, and latency.

#### Props Interface

```typescript
interface MetricsPanelProps {
  metrics: SearchMetrics | null;
}
```

Where `SearchMetrics` is:
```typescript
interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number; // milliseconds
}
```

#### State
- None (presentation component)

#### Behavior

1. **Display Logic**:
   - Only visible when `metrics !== null`
   - Hidden when no search has been performed

2. **Metrics Display**:
   - Query: "Query: {query}"
   - Result Count: "{count} result(s) found"
   - Latency: "Latency: {latency}ms" (formatted to 0 decimal places)

3. **Layout**:
   - Horizontal or vertical layout (designer's choice)
   - Subtle styling (not prominent, informational)

#### Rendering Logic

- Renders only if `metrics !== null`
- Returns `null` if `metrics === null`

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  Query Metrics                                      │
│                                                     │
│  Query: migration validation                       │
│  Results: 3 result(s) found                        │
│  Latency: 45ms                                      │
└─────────────────────────────────────────────────────┘
```

#### Accessibility

- Panel has `role="region"` and `aria-label="Search metrics"`
- Each metric has semantic HTML (e.g., `<dl>`, `<dt>`, `<dd>`)

---

### 3.6 ExampleQueries Component

#### Purpose
Provides clickable buttons with pre-defined example queries for quick testing.

#### Props Interface

```typescript
interface ExampleQueriesProps {
  onQuerySelect: (query: string) => void;
  isLoading: boolean;
}
```

#### State
- None (presentation component)

#### Behavior

1. **Query Buttons**:
   - Four buttons with pre-defined queries:
     - "migration" (single term)
     - "migration validation" (implicit AND)
     - "migration OR validation" (explicit OR)
     - "schema -migration" (NOT query)
   - Each button calls `onQuerySelect(query)` on click
   - Buttons disabled when `isLoading === true`

2. **Auto-Execute**:
   - When a button is clicked, parent (App.tsx) should:
     1. Set query state to selected query
     2. Automatically trigger search (call search API)

#### Rendering Logic

- Always renders (no conditional rendering)
- Buttons arranged horizontally or in a grid

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  Example Queries                                    │
│                                                     │
│  [migration]  [migration validation]               │
│  [migration OR validation]  [schema -migration]    │
└─────────────────────────────────────────────────────┘
```

#### Accessibility

- Each button has `aria-label="Search for: {query}"`
- Container has `role="group"` and `aria-label="Example queries"`

---

### 3.7 App Component (Root)

#### Purpose
Main orchestrator component that manages global state, coordinates API calls, and renders all child components.

#### Props Interface
- None (root component)

#### State

```typescript
interface AppState {
  query: string;
  limit: number;
  results: SearchResult[];
  metrics: SearchMetrics | null;
  error: string | null;
  isLoading: boolean;
  health: HealthStatus;
  healthLastChecked: Date | null;
  isHealthRefreshing: boolean;
}
```

#### Behavior

1. **Search Flow**:
   - User enters query and clicks "Search"
   - `handleSearch()` called:
     1. Set `isLoading = true`
     2. Set `error = null`
     3. Call `searchApi.search(query, limit)`
     4. On success: update `results`, `metrics`, `isLoading = false`
     5. On error: set `error`, `isLoading = false`

2. **Health Check Flow**:
   - On mount: start auto-refresh timer (5s interval)
   - Call `searchApi.checkHealth()` every 5 seconds
   - Update `health` and `healthLastChecked` on response
   - On unmount: clear timer

3. **Query Change**:
   - `handleQueryChange(query)` updates `query` state
   - Does NOT trigger search (user must click Search)

4. **Limit Change**:
   - `handleLimitChange(limit)` updates `limit` state
   - Does NOT trigger search (user must click Search)

5. **Clear**:
   - `handleClear()` resets:
     - `query = ""`
     - `results = []`
     - `metrics = null`
     - `error = null`

6. **Example Query Selection**:
   - `handleExampleQuery(query)` sets `query` state and immediately calls `handleSearch()`

#### Rendering Logic

**Layout Structure:**
```
<App>
  <Header>
    <Title />
    <HealthStatus />
  </Header>
  <Main>
    <SearchSection>
      <SearchBar />
      <ExampleQueries />
    </SearchSection>
    <MetricsPanel />
    <ResultsList />
  </Main>
  <Footer />
</App>
```

#### Visual Example (ASCII)

```
┌─────────────────────────────────────────────────────┐
│  Haystack Search Engine        ● Healthy  [Refresh]│
├─────────────────────────────────────────────────────┤
│                                                      │
│  [SearchBar Component]                              │
│  [ExampleQueries Component]                          │
│                                                      │
│  [MetricsPanel Component]                            │
│                                                      │
│  [ResultsList Component]                             │
│                                                      │
├─────────────────────────────────────────────────────┤
│  © 2026 Haystack Search Engine                      │
└─────────────────────────────────────────────────────┘
```

---

## 4. API Client Specification

### 4.1 Module: `services/searchApi.ts`

#### Purpose
Centralized API client using Axios for all backend communication. Handles request/response transformation, error handling, and timeout configuration.

#### Exported Functions

##### `search(query: string, k: number): Promise<SearchResponse>`

**Purpose**: Execute a search query against the backend.

**Parameters**:
- `query`: Search query string (required, non-empty)
- `k`: Result limit 1-50 (required, validated)

**Returns**: `Promise<SearchResponse>`

**Behavior**:
1. Validates `query` is non-empty (throws if empty)
2. Validates `k` is between 1 and 50 (throws if invalid)
3. Encodes query string for URL (e.g., "migration validation" → "migration%20validation")
4. Makes GET request to `http://localhost:8900/search?q={encodedQuery}&k={k}`
5. Sets timeout to 5000ms (5 seconds)
6. On success (200 OK):
   - Parses JSON response
   - Validates response structure (throws if invalid)
   - Returns `SearchResponse` object
7. On error:
   - Network error: throws `NetworkError` with message "Cannot connect to server..."
   - Timeout: throws `TimeoutError` with message "Request timeout..."
   - HTTP error (4xx/5xx): throws `HttpError` with status and message
   - Invalid response: throws `InvalidResponseError` with message

**Error Types**:
```typescript
class NetworkError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'NetworkError';
  }
}

class TimeoutError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'TimeoutError';
  }
}

class HttpError extends Error {
  status: number;
  constructor(status: number, message: string) {
    super(message);
    this.name = 'HttpError';
    this.status = status;
  }
}

class InvalidResponseError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidResponseError';
  }
}
```

##### `checkHealth(): Promise<HealthCheckResponse>`

**Purpose**: Check server health status.

**Parameters**: None

**Returns**: `Promise<HealthCheckResponse>`

**Behavior**:
1. Makes GET request to `http://localhost:8900/health`
2. Sets timeout to 2000ms (2 seconds, shorter than search)
3. On success (200 OK):
   - Response body should be "OK" (case-insensitive)
   - Returns `{ status: 'healthy' }`
4. On 503 Service Unavailable:
   - Returns `{ status: 'unhealthy' }`
5. On other errors:
   - Network/timeout: returns `{ status: 'unhealthy' }` (treat as unavailable)
   - Other HTTP errors: returns `{ status: 'unhealthy' }`

**Note**: Health check should never throw (always returns a status, even on error).

#### Configuration

**Base URL**: `http://localhost:8900`

**Axios Instance**:
```typescript
const apiClient = axios.create({
  baseURL: 'http://localhost:8900',
  timeout: 5000, // 5 seconds default
  headers: {
    'Content-Type': 'application/json',
    'Accept': 'application/json'
  }
});
```

**Timeout Overrides**:
- Search: 5000ms
- Health: 2000ms

#### Error Handling Strategy

1. **Network Errors** (ECONNREFUSED, ENOTFOUND):
   - Message: "Cannot connect to server. Please ensure searchd is running on port 8900."
   - Type: `NetworkError`

2. **Timeout Errors**:
   - Message: "Request timeout. Server may be overloaded."
   - Type: `TimeoutError`

3. **HTTP Errors** (4xx, 5xx):
   - Message: "Search failed with error: {status} {statusText}"
   - Type: `HttpError` (includes status code)

4. **Invalid Response** (non-JSON, missing fields):
   - Message: "Invalid server response. Please check server version."
   - Type: `InvalidResponseError`

#### Response Validation

**Search Response Validation**:
- Must have `query` (string)
- Must have `results` (array)
- Each result must have `docId` (number), `score` (number), `snippet` (string)
- Throws `InvalidResponseError` if validation fails

**Health Response Validation**:
- 200 OK: Body must be "OK" (case-insensitive, trimmed)
- 503: Body may be empty (acceptable)
- Other statuses: Treated as unhealthy

---

## 5. TypeScript Type Definitions

### 5.1 File: `types/search.ts`

#### Complete Type Definitions

```typescript
/**
 * Search Result Interface
 * Represents a single search result from the backend
 */
export interface SearchResult {
  /** Document identifier (integer) */
  docId: number;
  
  /** BM25 relevance score (floating point) */
  score: number;
  
  /** Text snippet containing query terms */
  snippet: string;
}

/**
 * Search Response Interface
 * Response from /search endpoint
 */
export interface SearchResponse {
  /** The original query string */
  query: string;
  
  /** Array of search results, sorted by score descending */
  results: SearchResult[];
}

/**
 * Search Metrics Interface
 * Client-side metrics for a search operation
 */
export interface SearchMetrics {
  /** The query that was executed */
  query: string;
  
  /** Number of results returned */
  resultCount: number;
  
  /** Request latency in milliseconds */
  latency: number;
}

/**
 * Health Status Type
 * Server health state
 */
export type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';

/**
 * Health Check Response Interface
 * Response from health check operation
 */
export interface HealthCheckResponse {
  /** Health status */
  status: HealthStatus;
}

/**
 * API Error Types
 */
export class NetworkError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'NetworkError';
    Object.setPrototypeOf(this, NetworkError.prototype);
  }
}

export class TimeoutError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'TimeoutError';
    Object.setPrototypeOf(this, TimeoutError.prototype);
  }
}

export class HttpError extends Error {
  status: number;
  constructor(status: number, message: string) {
    super(message);
    this.name = 'HttpError';
    this.status = status;
    Object.setPrototypeOf(this, HttpError.prototype);
  }
}

export class InvalidResponseError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidResponseError';
    Object.setPrototypeOf(this, InvalidResponseError.prototype);
  }
}

/**
 * Union type for all API errors
 */
export type ApiError = NetworkError | TimeoutError | HttpError | InvalidResponseError;
```

### 5.2 Type Safety Requirements

1. **Zero `any` types**: All variables, parameters, and return types must be explicitly typed
2. **Strict mode**: `tsconfig.json` must have `strict: true`
3. **No implicit any**: All function parameters must have types
4. **Generic constraints**: Use generics where appropriate (e.g., Axios response types)

### 5.3 Type Usage Examples

**Component Props**:
```typescript
interface SearchBarProps {
  query: string; // not any
  onSearch: () => void; // not any
}
```

**API Functions**:
```typescript
function search(query: string, k: number): Promise<SearchResponse> {
  // Return type explicitly declared
}
```

**State Hooks**:
```typescript
const [results, setResults] = useState<SearchResult[]>([]);
// Generic type parameter used
```

---

## 6. User Flows

### 6.1 Flow 1: User Searches for "migration" and Views Results

**Steps**:
1. User opens application
2. Application loads, health check runs automatically
3. User types "migration" in search input
4. User clicks "Search" button (or presses Enter)
5. Application shows loading spinner
6. API request sent to `/search?q=migration&k=10`
7. Backend responds with results
8. Application displays results in `ResultsList`
9. `MetricsPanel` shows query, result count, and latency

**State Transitions**:
```
Initial: query="", results=[], isLoading=false, error=null
  ↓ (user types)
query="migration", results=[], isLoading=false, error=null
  ↓ (user clicks Search)
query="migration", results=[], isLoading=true, error=null
  ↓ (API success)
query="migration", results=[...], isLoading=false, error=null, metrics={...}
```

**Success Criteria**:
- Results displayed correctly
- Metrics shown
- No errors
- Loading state transitions smoothly

---

### 6.2 Flow 2: User Clicks Example Query and Sees Results

**Steps**:
1. User opens application
2. User sees example query buttons
3. User clicks "migration validation" button
4. Application automatically:
   - Sets query to "migration validation"
   - Triggers search
5. Application shows loading spinner
6. API request sent to `/search?q=migration%20validation&k=10`
7. Backend responds with results
8. Application displays results

**State Transitions**:
```
Initial: query="", results=[]
  ↓ (user clicks example button)
query="migration validation", results=[], isLoading=true
  ↓ (API success)
query="migration validation", results=[...], isLoading=false
```

**Success Criteria**:
- Query input updates automatically
- Search triggers automatically
- Results display correctly

---

### 6.3 Flow 3: User Experiences Network Error

**Steps**:
1. User opens application
2. Backend server is not running (or port 8900 unavailable)
3. User enters query and clicks "Search"
4. Application shows loading spinner
5. API request fails with network error
6. Application catches error
7. Application displays error message: "Cannot connect to server. Please ensure searchd is running on port 8900."
8. Results list shows error state
9. Loading spinner disappears

**State Transitions**:
```
query="migration", results=[], isLoading=false, error=null
  ↓ (user clicks Search)
query="migration", results=[], isLoading=true, error=null
  ↓ (API network error)
query="migration", results=[], isLoading=false, error="Cannot connect..."
```

**Success Criteria**:
- Error message is user-friendly
- Error state is clearly visible
- User can retry after fixing server

---

### 6.4 Flow 4: Health Indicator Updates When Server Stops

**Steps**:
1. User opens application
2. Backend server is running
3. Health indicator shows "● Healthy"
4. Backend server stops (or crashes)
5. Next health check (within 5 seconds) fails
6. Health indicator updates to "● Unavailable"
7. Status text changes to "Unavailable"
8. Auto-refresh continues polling every 5 seconds
9. When server restarts, health indicator updates back to "● Healthy"

**State Transitions**:
```
health='healthy', healthLastChecked=Date
  ↓ (5 seconds pass, health check runs)
health='unhealthy', healthLastChecked=Date (new)
  ↓ (server restarts, 5 seconds pass)
health='healthy', healthLastChecked=Date (new)
```

**Success Criteria**:
- Health indicator updates automatically
- Status is accurate
- Auto-refresh continues working
- Manual refresh button works

---

### 6.5 Flow 5: User Adjusts Result Limit and Searches

**Steps**:
1. User opens application
2. User enters query "migration"
3. User adjusts slider to limit=25
4. Slider display updates: "Limit: 25"
5. User clicks "Search"
6. API request sent to `/search?q=migration&k=25`
7. Backend responds with up to 25 results
8. Application displays results (up to 25)

**State Transitions**:
```
query="migration", limit=10
  ↓ (user adjusts slider)
query="migration", limit=25
  ↓ (user clicks Search)
query="migration", limit=25, isLoading=true
  ↓ (API success)
query="migration", limit=25, results=[...up to 25], isLoading=false
```

**Success Criteria**:
- Slider updates limit value
- API request includes correct `k` parameter
- Results respect limit

---

## 7. Error Handling Strategy

### 7.1 Error Types and Handling

#### Network Errors

**Cause**: Server not running, port unavailable, DNS failure

**Detection**: Axios throws error with `code === 'ECONNREFUSED'` or `code === 'ENOTFOUND'`

**User Message**: "Cannot connect to server. Please ensure searchd is running on port 8900."

**Recovery**: User must start server, then retry

**UI Display**: Error state in `ResultsList`, red text, clear message

---

#### Timeout Errors

**Cause**: Server takes >5 seconds to respond

**Detection**: Axios throws error with `code === 'ECONNABORTED'`

**User Message**: "Request timeout. Server may be overloaded."

**Recovery**: User can retry, or check server logs

**UI Display**: Error state in `ResultsList`, red text, clear message

---

#### HTTP Errors (4xx, 5xx)

**Cause**: Server returns error status code

**Detection**: Axios response with `status >= 400`

**User Message**: "Search failed with error: {status} {statusText}"

**Recovery**: Depends on error (user may need to check query format, or server may need fixing)

**UI Display**: Error state in `ResultsList`, red text, status code shown

---

#### Invalid Response Errors

**Cause**: Server returns non-JSON, or JSON missing required fields

**Detection**: JSON parse error, or response validation fails

**User Message**: "Invalid server response. Please check server version."

**Recovery**: User may need to update server, or check server logs

**UI Display**: Error state in `ResultsList`, red text, clear message

---

### 7.2 Error Display Strategy

**Location**: `ResultsList` component shows error state

**Format**:
- Red text color (Tailwind: `text-red-500`)
- Clear, user-friendly message
- No technical jargon (e.g., "ECONNREFUSED" not shown)
- Actionable guidance when possible

**Example Error Display**:
```
┌─────────────────────────────────────────────────────┐
│  ⚠️  Cannot connect to server.                      │
│  Please ensure searchd is running on port 8900.    │
└─────────────────────────────────────────────────────┘
```

### 7.3 Logging Strategy

**Development Mode**:
- Log all errors to console with full details
- Include error stack traces
- Include request/response data

**Production Mode**:
- Log errors to console (for debugging)
- Do not expose sensitive information
- Consider future integration with error tracking service

**Log Format**:
```typescript
console.error('[SearchAPI]', {
  error: error.name,
  message: error.message,
  query: query, // if applicable
  timestamp: new Date().toISOString()
});
```

### 7.4 Recovery Actions

**User Actions**:
1. **Retry**: User can click "Search" again after fixing issue
2. **Clear**: User can clear query and start over
3. **Check Health**: User can check health status to verify server

**Automatic Actions**:
- Health check continues polling (may detect server recovery)
- No automatic retries for search (user must explicitly retry)

---

## 8. Test Plan

### 8.1 Testing Strategy

**Approach**: Test-Driven Development (TDD)

**Test Levels**:
1. **Unit Tests**: Individual components and functions
2. **Integration Tests**: Component interactions and API client
3. **E2E Tests**: Full user flows (optional, not in scope)

**Test Tools**:
- **Vitest**: Test runner
- **React Testing Library**: Component testing
- **Axios Mock Adapter**: API mocking
- **@testing-library/user-event**: User interaction simulation

**Coverage Goals**:
- Components: >80% coverage
- API Client: >90% coverage
- Utilities: >90% coverage
- Overall: >80% coverage

### 8.2 Test Case Enumeration

#### Component: SearchBar

**TC-001**: Renders with empty input field
- **Given**: Component mounted with `query=""`
- **Then**: Input field is empty, placeholder visible

**TC-002**: Submit button disabled when query is empty
- **Given**: Component mounted with `query=""`
- **Then**: Submit button has `disabled={true}`

**TC-003**: Submit button enabled when query has text
- **Given**: Component mounted with `query="test"`
- **Then**: Submit button has `disabled={false}`

**TC-004**: Submit button disabled when loading
- **Given**: Component mounted with `isLoading={true}`
- **Then**: Submit button has `disabled={true}`

**TC-005**: Calls onSearch when Enter key pressed
- **Given**: Component mounted, input has focus, query="test"
- **When**: User presses Enter key
- **Then**: `onSearch` called once

**TC-006**: Does not call onSearch when Enter pressed with empty query
- **Given**: Component mounted, input has focus, query=""
- **When**: User presses Enter key
- **Then**: `onSearch` not called

**TC-007**: Calls onSearch when submit button clicked
- **Given**: Component mounted, query="test"
- **When**: User clicks submit button
- **Then**: `onSearch` called once

**TC-008**: Calls onQueryChange when input changes
- **Given**: Component mounted
- **When**: User types "test" in input
- **Then**: `onQueryChange` called with "test"

**TC-009**: Clear button visible when query has text
- **Given**: Component mounted with `query="test"`
- **Then**: Clear button is visible

**TC-010**: Clear button hidden when query is empty
- **Given**: Component mounted with `query=""`
- **Then**: Clear button is not visible (or hidden)

**TC-011**: Calls onClear when clear button clicked
- **Given**: Component mounted, query="test"
- **When**: User clicks clear button
- **Then**: `onClear` called once

**TC-012**: Slider updates limit value
- **Given**: Component mounted with `limit=10`
- **When**: User moves slider to 25
- **Then**: `onLimitChange` called with 25

**TC-013**: Slider displays current limit
- **Given**: Component mounted with `limit=15`
- **Then**: Slider shows "Limit: 15"

**TC-014**: Slider respects min/max bounds (1-50)
- **Given**: Component mounted
- **When**: User tries to set limit < 1 or > 50
- **Then**: Limit clamped to 1 or 50

**TC-015**: Input disabled when loading
- **Given**: Component mounted with `isLoading={true}`
- **Then**: Input field has `disabled={true}`

---

#### Component: ResultsList

**TC-016**: Renders loading state when isLoading=true
- **Given**: Component mounted with `isLoading={true}`
- **Then**: Loading spinner and "Searching..." text visible

**TC-017**: Renders error state when error is not null
- **Given**: Component mounted with `error="Network error"`
- **Then**: Error message displayed, results hidden

**TC-018**: Renders empty state when no results and query exists
- **Given**: Component mounted with `results=[]`, `query="test"`, `isLoading=false`, `error=null`
- **Then**: "No results found..." message visible

**TC-019**: Does not render empty state when query is empty
- **Given**: Component mounted with `results=[]`, `query=""`
- **Then**: Empty state not shown (or shows initial state)

**TC-020**: Renders results when results array has items
- **Given**: Component mounted with `results=[{docId:1, score:2.5, snippet:"test"}]`
- **Then**: ResultCard components rendered for each result

**TC-021**: Results list shows correct count
- **Given**: Component mounted with `results=[...3 items]`
- **Then**: "Search Results (3 found)" or similar displayed

**TC-022**: Loading state takes priority over results
- **Given**: Component mounted with `isLoading=true`, `results=[...]`
- **Then**: Loading state shown, results hidden

**TC-023**: Error state takes priority over results
- **Given**: Component mounted with `error="Error"`, `results=[...]`
- **Then**: Error state shown, results hidden

---

#### Component: ResultCard

**TC-024**: Renders rank correctly
- **Given**: Component mounted with `rank=1`, `result={...}`
- **Then**: "#1" displayed

**TC-025**: Renders document ID correctly
- **Given**: Component mounted with `result={docId: 5, ...}`
- **Then**: "Document 5" displayed

**TC-026**: Renders score with 3 decimal places
- **Given**: Component mounted with `result={score: 2.456789, ...}`
- **Then**: "2.457" displayed (rounded to 3 decimals)

**TC-027**: Renders snippet text
- **Given**: Component mounted with `result={snippet: "Test snippet", ...}`
- **Then**: "Test snippet" displayed

**TC-028**: Handles long snippets (truncation or wrapping)
- **Given**: Component mounted with very long snippet
- **Then**: Snippet displayed (truncated or wrapped, design decision)

---

#### Component: HealthStatus

**TC-029**: Renders green dot when healthy
- **Given**: Component mounted with `health='healthy'`
- **Then**: Green dot (●) visible

**TC-030**: Renders red dot when unhealthy
- **Given**: Component mounted with `health='unhealthy'`
- **Then**: Red dot (●) visible

**TC-031**: Renders gray dot when unknown
- **Given**: Component mounted with `health='unknown'`
- **Then**: Gray dot (○) visible

**TC-032**: Displays "Healthy" text when healthy
- **Given**: Component mounted with `health='healthy'`
- **Then**: "Healthy" text displayed

**TC-033**: Displays "Unavailable" text when unhealthy
- **Given**: Component mounted with `health='unhealthy'`
- **Then**: "Unavailable" text displayed

**TC-034**: Displays last checked timestamp
- **Given**: Component mounted with `lastChecked=new Date()`
- **Then**: Relative time displayed (e.g., "2s ago")

**TC-035**: Displays "Never" when lastChecked is null
- **Given**: Component mounted with `lastChecked=null`
- **Then**: "Never" displayed

**TC-036**: Calls onRefresh when refresh button clicked
- **Given**: Component mounted
- **When**: User clicks refresh button
- **Then**: `onRefresh` called once

**TC-037**: Refresh button disabled when refreshing
- **Given**: Component mounted with `isRefreshing={true}`
- **Then**: Refresh button has `disabled={true}`

**TC-038**: Timestamp updates every second (client-side)
- **Given**: Component mounted with `lastChecked=Date(now - 5000)`
- **When**: 1 second passes
- **Then**: Timestamp updates to "6s ago" (or similar)

---

#### Component: MetricsPanel

**TC-039**: Renders when metrics is not null
- **Given**: Component mounted with `metrics={query:"test", resultCount:5, latency:45}`
- **Then**: Metrics panel visible

**TC-040**: Does not render when metrics is null
- **Given**: Component mounted with `metrics=null`
- **Then**: Component returns null (or nothing rendered)

**TC-041**: Displays query text
- **Given**: Component mounted with `metrics={query:"migration", ...}`
- **Then**: "Query: migration" displayed

**TC-042**: Displays result count
- **Given**: Component mounted with `metrics={resultCount:3, ...}`
- **Then**: "3 result(s) found" displayed

**TC-043**: Displays latency in milliseconds
- **Given**: Component mounted with `metrics={latency:123, ...}`
- **Then**: "Latency: 123ms" displayed

**TC-044**: Handles plural/singular for result count
- **Given**: Component mounted with `metrics={resultCount:1, ...}`
- **Then**: "1 result found" (singular) displayed

---

#### Component: ExampleQueries

**TC-045**: Renders all four example query buttons
- **Given**: Component mounted
- **Then**: Four buttons visible with correct labels

**TC-046**: Calls onQuerySelect with correct query when button clicked
- **Given**: Component mounted
- **When**: User clicks "migration" button
- **Then**: `onQuerySelect` called with "migration"

**TC-047**: Buttons disabled when loading
- **Given**: Component mounted with `isLoading={true}`
- **Then**: All buttons have `disabled={true}`

**TC-048**: Buttons enabled when not loading
- **Given**: Component mounted with `isLoading={false}`
- **Then**: All buttons have `disabled={false}`

---

#### API Client: searchApi.ts

**TC-049**: search() calls /search with encoded query string
- **Given**: API client configured
- **When**: `search("migration validation", 10)` called
- **Then**: GET request to `/search?q=migration%20validation&k=10`

**TC-050**: search() includes k parameter in request
- **Given**: API client configured
- **When**: `search("test", 25)` called
- **Then**: Request includes `k=25` parameter

**TC-051**: search() throws NetworkError on connection failure
- **Given**: Server not running
- **When**: `search("test", 10)` called
- **Then**: `NetworkError` thrown with appropriate message

**TC-052**: search() throws TimeoutError on timeout
- **Given**: Server configured to delay >5s
- **When**: `search("test", 10)` called
- **Then**: `TimeoutError` thrown after 5 seconds

**TC-053**: search() throws HttpError on 4xx/5xx response
- **Given**: Server returns 500
- **When**: `search("test", 10)` called
- **Then**: `HttpError` thrown with status 500

**TC-054**: search() throws InvalidResponseError on invalid JSON
- **Given**: Server returns non-JSON response
- **When**: `search("test", 10)` called
- **Then**: `InvalidResponseError` thrown

**TC-055**: search() throws InvalidResponseError on missing fields
- **Given**: Server returns JSON without `results` field
- **When**: `search("test", 10)` called
- **Then**: `InvalidResponseError` thrown

**TC-056**: search() validates query is non-empty
- **Given**: API client configured
- **When**: `search("", 10)` called
- **Then**: Error thrown before API call

**TC-057**: search() validates k is between 1 and 50
- **Given**: API client configured
- **When**: `search("test", 0)` or `search("test", 51)` called
- **Then**: Error thrown before API call

**TC-058**: search() returns SearchResponse on success
- **Given**: Server returns valid response
- **When**: `search("test", 10)` called
- **Then**: `SearchResponse` object returned with correct structure

**TC-059**: checkHealth() returns healthy when server returns 200 OK
- **Given**: Server returns 200 with "OK" body
- **When**: `checkHealth()` called
- **Then**: Returns `{status: 'healthy'}`

**TC-060**: checkHealth() returns unhealthy when server returns 503
- **Given**: Server returns 503
- **When**: `checkHealth()` called
- **Then**: Returns `{status: 'unhealthy'}`

**TC-061**: checkHealth() returns unhealthy on network error
- **Given**: Server not running
- **When**: `checkHealth()` called
- **Then**: Returns `{status: 'unhealthy'}` (does not throw)

**TC-062**: checkHealth() returns unhealthy on timeout
- **Given**: Server configured to delay >2s
- **When**: `checkHealth()` called
- **Then**: Returns `{status: 'unhealthy'}` (does not throw)

**TC-063**: checkHealth() uses shorter timeout than search
- **Given**: API client configured
- **When**: `checkHealth()` called
- **Then**: Timeout is 2000ms (not 5000ms)

---

#### Integration: User Flows

**TC-064**: User can search and see results end-to-end
- **Given**: Server running, app loaded
- **When**: User enters query, clicks Search
- **Then**: Results displayed, metrics shown, no errors

**TC-065**: Error message displays when server unavailable
- **Given**: Server not running, app loaded
- **When**: User enters query, clicks Search
- **Then**: Error message displayed in ResultsList

**TC-066**: Example query auto-executes search
- **Given**: Server running, app loaded
- **When**: User clicks example query button
- **Then**: Query set, search triggered, results displayed

**TC-067**: Health indicator updates automatically
- **Given**: App loaded, server running
- **When**: Server stops
- **Then**: Health indicator updates to unhealthy within 5 seconds

**TC-068**: Limit slider affects search results
- **Given**: Server running, app loaded
- **When**: User sets limit to 25, searches
- **Then**: API called with k=25, results respect limit

**TC-069**: Clear button resets state
- **Given**: App loaded, search performed, results shown
- **When**: User clicks Clear
- **Then**: Query empty, results cleared, metrics cleared

**TC-070**: Loading state shows during API call
- **Given**: Server running, app loaded
- **When**: User searches (with slow server simulation)
- **Then**: Loading spinner visible during request

---

#### Utilities: formatters.ts

**TC-071**: formatScore() formats to 3 decimal places
- **Given**: Score = 2.456789
- **When**: `formatScore(score)` called
- **Then**: Returns "2.457"

**TC-072**: formatTimestamp() formats relative time
- **Given**: Date 5 seconds ago
- **When**: `formatTimestamp(date)` called
- **Then**: Returns "5s ago"

**TC-073**: formatTimestamp() handles minutes
- **Given**: Date 2 minutes ago
- **When**: `formatTimestamp(date)` called
- **Then**: Returns "2m ago"

---

#### Utilities: validators.ts

**TC-074**: validateQuery() returns true for non-empty string
- **Given**: Query = "test"
- **When**: `validateQuery(query)` called
- **Then**: Returns true

**TC-075**: validateQuery() returns false for empty string
- **Given**: Query = ""
- **When**: `validateQuery(query)` called
- **Then**: Returns false

**TC-076**: validateQuery() returns false for whitespace-only
- **Given**: Query = "   "
- **When**: `validateQuery(query)` called
- **Then**: Returns false

**TC-077**: validateLimit() returns true for 1-50
- **Given**: Limit = 25
- **When**: `validateLimit(limit)` called
- **Then**: Returns true

**TC-078**: validateLimit() returns false for <1 or >50
- **Given**: Limit = 0 or 51
- **When**: `validateLimit(limit)` called
- **Then**: Returns false

---

### 8.3 Test File Organization

```
src/
├── components/
│   ├── SearchBar.test.tsx
│   ├── ResultsList.test.tsx
│   ├── ResultCard.test.tsx
│   ├── HealthStatus.test.tsx
│   ├── MetricsPanel.test.tsx
│   └── ExampleQueries.test.tsx
├── services/
│   └── searchApi.test.ts
├── utils/
│   ├── formatters.test.ts
│   └── validators.test.ts
└── App.test.tsx (integration tests)
```

### 8.4 Mocking Strategy

**API Mocking**:
- Use `axios-mock-adapter` for all API calls
- Mock successful responses
- Mock error responses (network, timeout, HTTP errors)
- Mock invalid responses

**Component Mocking**:
- Mock child components when testing parent
- Use `jest.mock()` for external dependencies
- Mock hooks if needed (rare, prefer testing hooks separately)

**Timer Mocking**:
- Use Vitest's `vi.useFakeTimers()` for health check polling
- Advance timers in tests to verify auto-refresh

---

## 9. UI Mockups

### 9.1 Initial State (Before Search)

```
┌─────────────────────────────────────────────────────────────────┐
│  Haystack Search Engine                    ● Healthy  [Refresh] │
│                                            Last checked: 3s ago  │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Query                                            │   │
│  │ ┌───────────────────────────────────────────────────┐   │   │
│  │ │ Enter your search query...                  [X] │   │   │
│  │ └───────────────────────────────────────────────────┘   │   │
│  │                                                          │   │
│  │ Result Limit: [━━━━━━━━●━━━━━━━━] 10                    │   │
│  │                                                          │   │
│  │ [Search]                          [Clear]               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
│  Example Queries:                                                │
│  [migration]  [migration validation]                            │
│  [migration OR validation]  [schema -migration]                 │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Results                                          │   │
│  │                                                          │   │
│  │  (Empty - no search performed yet)                      │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
├─────────────────────────────────────────────────────────────────┤
│  © 2026 Haystack Search Engine                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 Loading State

```
┌─────────────────────────────────────────────────────────────────┐
│  Haystack Search Engine                    ● Healthy  [Refresh] │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  [SearchBar with query="migration", submit disabled]             │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Results                                          │   │
│  │                                                          │   │
│  │              ⏳                                           │   │
│  │         Searching...                                      │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 9.3 Success State (With Results)

```
┌─────────────────────────────────────────────────────────────────┐
│  Haystack Search Engine                    ● Healthy  [Refresh] │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  [SearchBar with query="migration validation"]                   │
│                                                                   │
│  Query Metrics:                                                  │
│  Query: migration validation  |  Results: 3 found  |  Latency: 45ms │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Results (3 found)                               │   │
│  │                                                          │   │
│  │ ┌─────────────────────────────────────────────────────┐ │   │
│  │ │ #1  Document 1                    Score: 2.456      │ │   │
│  │ │ ─────────────────────────────────────────────────── │ │   │
│  │ │ Teamcenter migration guide: map attributes,        │ │   │
│  │ │ validate schema, run dry-run.                       │ │   │
│  │ └─────────────────────────────────────────────────────┘ │   │
│  │                                                          │   │
│  │ ┌─────────────────────────────────────────────────────┐ │   │
│  │ │ #2  Document 2                    Score: 1.789       │ │   │
│  │ │ ─────────────────────────────────────────────────── │ │   │
│  │ │ PLM data migration: cleansing, mapping, validation.│ │   │
│  │ └─────────────────────────────────────────────────────┘ │   │
│  │                                                          │   │
│  │ ┌─────────────────────────────────────────────────────┐ │   │
│  │ │ #3  Document 3                    Score: 1.234     │ │   │
│  │ │ ─────────────────────────────────────────────────── │ │   │
│  │ │ Schema validation: ensure data integrity.           │ │   │
│  │ └─────────────────────────────────────────────────────┘ │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 9.4 Empty State (No Results)

```
┌─────────────────────────────────────────────────────────────────┐
│  Haystack Search Engine                    ● Healthy  [Refresh] │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  [SearchBar with query="xyzabc123"]                               │
│                                                                   │
│  Query Metrics:                                                  │
│  Query: xyzabc123  |  Results: 0 found  |  Latency: 32ms        │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Results                                          │   │
│  │                                                          │   │
│  │  No results found. Try a different query.               │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 9.5 Error State

```
┌─────────────────────────────────────────────────────────────────┐
│  Haystack Search Engine                    ● Unavailable [Refresh] │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  [SearchBar with query="migration"]                               │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Search Results                                          │   │
│  │                                                          │   │
│  │  ⚠️  Cannot connect to server.                          │   │
│  │  Please ensure searchd is running on port 8900.          │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 9.6 Color Scheme (Tailwind CSS)

- **Primary**: `blue-600` (buttons, links)
- **Success**: `green-500` (healthy status)
- **Error**: `red-500` (error messages, unhealthy status)
- **Background**: `gray-50` (page background)
- **Card Background**: `white` (result cards)
- **Border**: `gray-200` (card borders)
- **Text Primary**: `gray-900` (headings, body)
- **Text Secondary**: `gray-600` (labels, timestamps)

### 9.7 Typography

- **Title**: `text-3xl font-bold` (Haystack Search Engine)
- **Section Headings**: `text-xl font-semibold` (Search Results, etc.)
- **Body Text**: `text-base` (default)
- **Small Text**: `text-sm` (timestamps, labels)
- **Code/Query**: `font-mono` (query text in metrics)

### 9.8 Responsive Design

**Mobile (< 640px)**:
- Single column layout
- Stacked components
- Full-width buttons
- Smaller font sizes

**Tablet (640px - 1024px)**:
- Two-column layout where appropriate
- Adjusted spacing

**Desktop (> 1024px)**:
- Max-width container (max-w-4xl)
- Centered layout
- Optimal spacing

---

## 10. Development Plan

### 10.1 Phase 2 GUI - 1: Setup and Configuration

**Duration**: 1-2 hours

**Tasks**:
1. Initialize Vite project with React + TypeScript template
2. Install dependencies:
   - `react`, `react-dom`
   - `axios`
   - `tailwindcss`, `autoprefixer`, `postcss`
   - `vitest`, `@testing-library/react`, `@testing-library/user-event`, `@testing-library/jest-dom`
   - `axios-mock-adapter` (for testing)
3. Configure `tsconfig.json` with strict mode
4. Configure `vite.config.ts` for Vitest
5. Configure Tailwind CSS
6. Set up folder structure (components, services, types, hooks, utils)
7. Create initial `App.tsx` and `main.tsx` (minimal, just renders "Hello World")

**Deliverable**: Project scaffolded, dependencies installed, configuration complete

**Acceptance**: Project builds, tests run, Tailwind works

---

### 10.2 Phase 2 GUI - 2: Write Test Files (TDD Red)

**Duration**: 4-6 hours

**Tasks**:
1. Write all test cases for `searchApi.ts` (TC-049 through TC-063)
2. Write all test cases for `formatters.ts` (TC-071 through TC-073)
3. Write all test cases for `validators.ts` (TC-074 through TC-078)
4. Write all test cases for `SearchBar.tsx` (TC-001 through TC-015)
5. Write all test cases for `ResultsList.tsx` (TC-016 through TC-023)
6. Write all test cases for `ResultCard.tsx` (TC-024 through TC-028)
7. Write all test cases for `HealthStatus.tsx` (TC-029 through TC-038)
8. Write all test cases for `MetricsPanel.tsx` (TC-039 through TC-044)
9. Write all test cases for `ExampleQueries.tsx` (TC-045 through TC-048)
10. Write integration tests for `App.tsx` (TC-064 through TC-070)

**Deliverable**: All test files created with failing tests (TDD Red phase)

**Acceptance**: All tests fail (expected), but test structure is correct

---

### 10.3 Phase 2 GUI - 3: Implement API Client (TDD Green)

**Duration**: 2-3 hours

**Tasks**:
1. Implement `types/search.ts` with all interfaces
2. Implement `services/searchApi.ts`:
   - `search()` function
   - `checkHealth()` function
   - Error classes (NetworkError, TimeoutError, HttpError, InvalidResponseError)
   - Response validation
3. Run tests, fix until all API client tests pass (TC-049 through TC-063)

**Deliverable**: API client fully implemented, all API tests passing

**Acceptance**: All API client tests green, can make real API calls (if server running)

---

### 10.4 Phase 2 GUI - 4: Implement Utilities (TDD Green)

**Duration**: 1 hour

**Tasks**:
1. Implement `utils/formatters.ts`:
   - `formatScore(score: number): string`
   - `formatTimestamp(date: Date): string`
2. Implement `utils/validators.ts`:
   - `validateQuery(query: string): boolean`
   - `validateLimit(limit: number): boolean`
3. Run tests, fix until all utility tests pass (TC-071 through TC-078)

**Deliverable**: Utilities implemented, all utility tests passing

**Acceptance**: All utility tests green

---

### 10.5 Phase 2 GUI - 5: Implement Components (TDD Green)

**Duration**: 6-8 hours

**Tasks**:
1. Implement `hooks/useSearch.ts` (custom hook for search logic)
2. Implement `hooks/useHealthCheck.ts` (custom hook for health monitoring)
3. Implement `hooks/useDebounce.ts` (if needed for debouncing)
4. Implement `components/LoadingSpinner.tsx` (reusable spinner)
5. Implement `components/SearchBar.tsx` (TC-001 through TC-015)
6. Implement `components/ResultsList.tsx` (TC-016 through TC-023)
7. Implement `components/ResultCard.tsx` (TC-024 through TC-028)
8. Implement `components/HealthStatus.tsx` (TC-029 through TC-038)
9. Implement `components/MetricsPanel.tsx` (TC-039 through TC-044)
10. Implement `components/ExampleQueries.tsx` (TC-045 through TC-048)
11. Implement `App.tsx` (orchestrator, integration)
12. Run tests, fix until all component tests pass

**Deliverable**: All components implemented, all component tests passing

**Acceptance**: All component tests green, UI renders correctly

---

### 10.6 Phase 2 GUI - 6: Integration and Polish (TDD Refactor)

**Duration**: 2-3 hours

**Tasks**:
1. Run all tests, ensure 100% pass rate
2. Check test coverage (aim for >80% overall)
3. Refactor code for:
   - Code quality (extract constants, improve naming)
   - Performance (memoization where needed)
   - Accessibility (ARIA labels, keyboard navigation)
4. Style with Tailwind CSS (match mockups)
5. Test with real backend server
6. Fix any integration issues
7. Verify all user flows work end-to-end

**Deliverable**: Polished application, all tests passing, ready for review

**Acceptance**: All acceptance criteria met

---

### 10.7 Phase 2 GUI - 7: Documentation

**Duration**: 1 hour

**Tasks**:
1. Write `README.md` with:
   - Setup instructions
   - Development workflow
   - Testing instructions
   - API documentation reference
2. Add code comments for complex logic
3. Document any deviations from spec

**Deliverable**: Complete documentation

**Acceptance**: Developer can set up and run project using README

---

## 11. Acceptance Criteria

### 11.1 Functional Requirements

- [ ] **FR-1**: Search query execution works (input, submit, Enter key, clear, limit slider)
- [ ] **FR-2**: Search results display correctly (rank, docId, score, snippet, empty state)
- [ ] **FR-3**: Server health monitoring works (indicator, auto-refresh, manual refresh, timestamp)
- [ ] **FR-4**: Query metrics display after each search (query, count, latency)
- [ ] **FR-5**: Example queries work (buttons clickable, auto-execute)
- [ ] **FR-6**: Error handling works (network, timeout, HTTP, invalid response)

### 11.2 Non-Functional Requirements

- [ ] **NFR-1**: Technology stack correct (React 18+, TypeScript, Vite, Axios, Tailwind)
- [ ] **NFR-2**: Code organization matches specified folder structure
- [ ] **NFR-3**: TypeScript strict mode, zero `any` types
- [ ] **NFR-4**: Test coverage >80% components, >90% API client
- [ ] **NFR-5**: Accessibility (keyboard nav, ARIA labels, screen reader)
- [ ] **NFR-6**: Performance (page load <2s, results render <100ms)
- [ ] **NFR-7**: Visual design matches mockups (colors, typography, responsive)

### 11.3 Test Requirements

- [ ] All 78+ test cases written and passing
- [ ] Tests use mocked APIs (no real backend required)
- [ ] Tests follow TDD approach (tests written before implementation)
- [ ] Integration tests verify user flows

### 11.4 Code Quality

- [ ] Code compiles with TypeScript strict mode
- [ ] No ESLint errors (if ESLint configured)
- [ ] Code follows React best practices
- [ ] Components are reusable and well-structured
- [ ] Error handling is comprehensive

### 11.5 User Experience

- [ ] Application is intuitive and easy to use
- [ ] Loading states provide feedback
- [ ] Error messages are clear and actionable
- [ ] Application is responsive (mobile, tablet, desktop)
- [ ] Application is accessible (keyboard, screen reader)

---

## 12. Open Questions / Assumptions

### 12.1 Assumptions

1. **Backend Server**: Assumes `searchd` is running on `http://localhost:8900` (not configurable in Phase 2)
2. **Browser Support**: Modern browsers only (Chrome, Firefox, Safari, Edge - latest 2 versions)
3. **Network**: Assumes localhost development (no CORS issues)
4. **Data**: Assumes backend has sample data (3 documents about Teamcenter, PLM, schema)
5. **Query Highlighting**: Snippet highlighting is optional enhancement (not in Phase 2 scope)

### 12.2 Open Questions

1. **Query Highlighting**: Should query terms be highlighted in snippets? (Assumed: No for Phase 2)
2. **Result Pagination**: Backend handles via `k` parameter, but should UI support "Load More"? (Assumed: No for Phase 2)
3. **Search History**: Should previous searches be saved? (Assumed: No for Phase 2)
4. **Health Check Frequency**: 5 seconds is reasonable? (Assumed: Yes, but can be adjusted)
5. **Error Retry**: Should there be automatic retry on error? (Assumed: No, user must manually retry)
6. **Mobile Layout**: Exact breakpoints for responsive design? (Assumed: Tailwind defaults)
7. **Loading Spinner**: Custom spinner or CSS-only? (Assumed: CSS-only with Tailwind)
8. **Timestamp Format**: Relative time only, or also absolute? (Assumed: Relative only, e.g., "2s ago")

### 12.3 Decisions Needed

1. **State Management**: Confirmed - React Hooks only (no Redux/Zustand)
2. **Styling Approach**: Confirmed - Tailwind CSS utility classes
3. **Testing Framework**: Confirmed - Vitest + React Testing Library
4. **API Mocking**: Confirmed - axios-mock-adapter for tests
5. **Type Safety**: Confirmed - Strict TypeScript, zero `any` types

### 12.4 Future Enhancements (Out of Scope)

- Dark mode
- Search history
- Saved queries
- Result export
- Advanced filters
- Multi-language support
- User preferences
- Analytics tracking

---

## Appendix A: Backend API Reference

### A.1 Search Endpoint

**URL**: `GET /search`

**Query Parameters**:
- `q` (required): Search query string (URL-encoded)
- `k` (optional): Result limit 1-50 (default: 10)

**Success Response (200 OK)**:
```json
{
  "query": "migration validation",
  "results": [
    {
      "docId": 1,
      "score": 2.456,
      "snippet": "Teamcenter migration guide: map attributes, validate schema, run dry-run."
    },
    {
      "docId": 2,
      "score": 1.789,
      "snippet": "PLM data migration: cleansing, mapping, validation."
    }
  ]
}
```

**Error Responses**:
- `400 Bad Request`: Invalid query or parameters
- `500 Internal Server Error`: Server error
- `503 Service Unavailable`: Server not ready

### A.2 Health Endpoint

**URL**: `GET /health`

**Query Parameters**: None

**Success Response (200 OK)**:
- Body: `"OK"` (case-insensitive)
- Content-Type: `text/plain` or `application/json`

**Unavailable Response (503 Service Unavailable)**:
- Body: Empty or error message
- Indicates server is not ready or shutting down

---

## Appendix B: Testability Considerations

### B.1 Timing-Dependent Behavior

- **Health Check Polling**: Tests use `vi.useFakeTimers()` to control time
- **Timestamp Updates**: Client-side timer updates every second (testable with fake timers)
- **API Timeouts**: Tests mock timeout behavior (no real 5-second waits)

### B.2 Test Environment Requirements

- **No Real Backend**: All tests use mocked APIs (axios-mock-adapter)
- **Isolated Tests**: Each test is independent (no shared state)
- **Fast Execution**: All tests should run in <10 seconds total

### B.3 Known Test Limitations

- **Real Network Calls**: Integration tests with real server are optional (not required)
- **Browser-Specific**: Some tests may behave differently in different browsers (use jsdom)
- **Timer Precision**: Fake timers may not perfectly match real timers (acceptable)

---

## Appendix C: Development Workflow

### C.1 TDD Workflow

1. **Write Test**: Create test file, write failing test
2. **Run Test**: Verify test fails (Red)
3. **Write Code**: Implement minimal code to pass test
4. **Run Test**: Verify test passes (Green)
5. **Refactor**: Improve code while keeping tests green
6. **Repeat**: Move to next test

### C.2 Git Workflow (Suggested)

1. Create feature branch: `git checkout -b feature/search-bar`
2. Write tests first (commit: "Add SearchBar tests")
3. Implement component (commit: "Implement SearchBar component")
4. Refactor if needed (commit: "Refactor SearchBar")
5. Merge to main when all tests pass

### C.3 Running Tests

```bash
# Run all tests
npm test

# Run tests in watch mode
npm test -- --watch

# Run tests with coverage
npm test -- --coverage

# Run specific test file
npm test -- SearchBar.test.tsx
```

### C.4 Running Development Server

```bash
# Start dev server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-XX | Architecture Team | Initial specification |

---

## Approval

**Status**: ✅ Approved

**Reviewers**:
- [x] Solution Architect (AI Review)
- [ ] Frontend Lead (Pending)
- [ ] QA Lead (Pending)

**Review Summary**:
The specification document has been comprehensively reviewed and is **APPROVED** for implementation. The document is:
- ✅ Complete: All required sections present (12 sections + 3 appendices)
- ✅ Detailed: 78+ test cases enumerated, component specs complete
- ✅ Clear: TypeScript interfaces, props, behavior all well-defined
- ✅ Testable: TDD approach specified, testability considerations included
- ✅ Consistent: Naming conventions, folder structure, technology stack aligned
- ✅ Professional: Well-formatted, proper grammar, comprehensive coverage

**Approval Date**: 2026-01-XX

**Approved By**: AI Architecture Review (Auto)

---

**END OF SPECIFICATION**
