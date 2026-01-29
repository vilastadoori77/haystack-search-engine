# Phase 2 GUI - Custom Hooks Master Specification

**Project:** Haystack Search Engine - React Frontend  
**Document Type:** Master Technical Specification  
**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Draft  
**Author:** Technical Specification Team  

---

## Document Control

| Version | Date | Author | Status | Changes |
|---------|------|--------|--------|---------|
| 1.0 | 2026-01-28 | Spec Team | Draft | Initial master specification |

---

## Table of Contents

1. [Overview](#1-overview)
2. [Specification Documents](#2-specification-documents)
3. [Hook Summary](#3-hook-summary)
4. [Implementation Order](#4-implementation-order)
5. [Testing Strategy](#5-testing-strategy)
6. [Refactoring Plan](#6-refactoring-plan)
7. [Success Criteria](#7-success-criteria)

---

## 1. Overview

### 1.1 Purpose

This master specification document provides an overview of all three custom hooks to be implemented, their relationships, and the overall implementation strategy.

### 1.2 Hooks Overview

| Hook | Purpose | Lines of Code | Test Cases |
|------|---------|--------------|------------|
| `useSearch` | Search state & API management | ~100 | 18 |
| `useHealthCheck` | Backend health monitoring | ~80 | 20 |
| `useDebounce` | Value debouncing utility | ~30 | 14 |
| **Total** | | **~210** | **52** |

### 1.3 Document Structure

This master spec references three detailed specifications:
1. `useSearch_SPEC.md` - Search functionality hook
2. `useHealthCheck_SPEC.md` - Health monitoring hook
3. `useDebounce_SPEC.md` - Debouncing utility hook

---

## 2. Specification Documents

### 2.1 Detailed Specifications

**useSearch Hook:**
- **File:** `docs/specs/useSearch_SPEC.md`
- **Purpose:** Manages search state, API calls, and error handling
- **Complexity:** High
- **Dependencies:** searchApi service, validators utility

**useHealthCheck Hook:**
- **File:** `docs/specs/useHealthCheck_SPEC.md`
- **Purpose:** Monitors backend health with periodic polling
- **Complexity:** Medium
- **Dependencies:** searchApi service

**useDebounce Hook:**
- **File:** `docs/specs/useDebounce_SPEC.md`
- **Purpose:** Generic debouncing utility
- **Complexity:** Low
- **Dependencies:** None (pure utility)

### 2.2 Related Documents

- **Requirements:** `docs/requirements/PHASE_2_GUI_HOOKS_REQUIREMENTS.md`
- **Architecture:** `docs/requirements/MICROSERVICES_ARCHITECTURE.md`
- **API Contract:** `docs/requirements/API_CONTRACT.md` (to be created)

---

## 3. Hook Summary

### 3.1 useSearch Hook

**Key Features:**
- Manages query, limit, results, metrics, error, loading states
- Executes search API calls
- Calculates latency metrics
- Handles all error types gracefully
- Provides clearSearch functionality

**State Managed:**
- `query: string`
- `limit: number` (1-50)
- `results: SearchResult[]`
- `metrics: SearchMetrics | null`
- `error: string | null`
- `isLoading: boolean`

**Functions Provided:**
- `setQuery(query: string): void`
- `setLimit(limit: number): void`
- `executeSearch(): Promise<void>`
- `clearSearch(): void`

### 3.2 useHealthCheck Hook

**Key Features:**
- Monitors backend health status
- Polls health endpoint every 5 seconds
- Provides manual refresh capability
- Tracks last checked timestamp
- Never throws exceptions

**State Managed:**
- `health: 'healthy' | 'unhealthy' | 'unknown'`
- `lastChecked: Date | null`
- `isRefreshing: boolean`

**Functions Provided:**
- `refreshHealth(): Promise<void>`

### 3.3 useDebounce Hook

**Key Features:**
- Generic debouncing utility
- Works with any value type
- Configurable delay
- Prevents memory leaks

**Parameters:**
- `value: T` (generic type)
- `delay: number` (milliseconds)

**Returns:**
- `T` (debounced value)

---

## 4. Implementation Order

### 4.1 Recommended Order

**Phase 1: Utility Hook (Simplest)**
1. Implement `useDebounce` hook
2. Write tests for `useDebounce`
3. Verify tests pass

**Phase 2: Health Monitoring Hook**
1. Implement `useHealthCheck` hook
2. Write tests for `useHealthCheck`
3. Verify tests pass

**Phase 3: Search Hook (Most Complex)**
1. Implement `useSearch` hook
2. Write tests for `useSearch`
3. Verify tests pass

**Alternative Order:**
You may implement useSearch before useHealthCheck if preferred. useDebounce should always be first (simplest). The key is building from simple to complex.

**Phase 4: Integration**
1. Refactor App.tsx to use all hooks
2. Verify all existing tests pass
3. Verify no functional changes

### 4.2 Rationale

- Start with simplest hook (useDebounce) to establish patterns
- Build complexity gradually
- Each hook can be tested independently
- Integration happens last

---

## 5. Testing Strategy

### 5.1 Test-Driven Development (TDD)

**Red-Green-Refactor Cycle:**
1. **Red:** Write tests first (all fail)
2. **Green:** Implement hook (all tests pass)
3. **Refactor:** Improve code (tests still pass)

### 5.2 Test Structure

**For Each Hook:**
- Test file: `src/hooks/{hookName}.test.ts`
- Test coverage: >95%
- Given-When-Then structure
- Comprehensive edge case coverage

### 5.3 Test Requirements Summary

**useDebounce Tests (14 tests):**
- Initial value
- Delay behavior
- Timer reset
- Cleanup
- Type safety
- Edge cases

**useHealthCheck Tests (20 tests):**
- Initial state
- Polling behavior
- Manual refresh
- Error handling
- Cleanup
- Recovery detection

**useSearch Tests (18 tests):**
- State management
- Query validation
- Limit validation
- Search execution
- Error handling
- Metrics calculation
- Clear functionality

---

## 6. Refactoring Plan

### 6.1 App.tsx Refactoring

**Current State:**
- ~216 lines
- Mixed concerns (state + UI)
- Hard to test logic independently

**Target State:**
- <60 lines
- UI composition only
- Logic in hooks

### 6.2 Refactoring Steps

**Step 1: Extract useSearch**
```typescript
// Before: State management in App.tsx
const [query, setQuery] = useState('');
const [results, setResults] = useState([]);
// ... more state

// After: Use hook
const {
  query,
  results,
  setQuery,
  executeSearch,
  // ...
} = useSearch();
```

**Step 2: Extract useHealthCheck**
```typescript
// Before: Health logic in App.tsx
const [health, setHealth] = useState('unknown');
useEffect(() => {
  // polling logic
}, []);

// After: Use hook
const {
  health,
  lastChecked,
  refreshHealth,
} = useHealthCheck();
```

**Step 3: Optional - Integrate useDebounce**
Note: This step is OPTIONAL for Phase 2. It demonstrates good practice but is not required for basic functionality.

```typescript
// For auto-search on typing
const debouncedQuery = useDebounce(query, 300);
useEffect(() => {
  if (debouncedQuery.trim()) {
    executeSearch();
  }
}, [debouncedQuery]);
```

### 6.3 Refactoring Checklist

- [ ] All hooks implemented and tested
- [ ] App.tsx refactored to use hooks
- [ ] All existing tests pass
- [ ] No functional changes
- [ ] No visual changes
- [ ] Code review completed

---

## 7. Common Pitfalls & Implementation Notes

### 7.1 Common Mistakes to Avoid

**useSearch:**
- Forgetting to clear results before new search
- Not handling empty/whitespace queries
- Missing cleanup for in-flight requests
- Forgetting to set isLoading to false in error case

**useHealthCheck:**
- Not checking mounted state before setState
- Forgetting to clear interval on unmount
- Setting isRefreshing during auto-polls (should only be manual)
- Not preventing state updates after unmount

**useDebounce:**
- Not clearing timeout in cleanup
- Forgetting to handle component unmount
- Using very short delays (<16ms)

### 7.2 Verification Steps

After implementing each hook:
1. Run tests: `npm test -- {hookName}.test.ts`
2. Check TypeScript: `npx tsc --noEmit`
3. Verify no console warnings
4. Check test coverage: `npm test -- --coverage {hookName}.test.ts`

---

## 8. Success Criteria

### 8.1 Code Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| App.tsx lines | <60 | Line count |
| Hook test coverage | >95% | Coverage report |
| TypeScript errors | 0 | `npx tsc --noEmit` |
| Test pass rate | 100% | `npm test` |

### 8.2 Functional Criteria

- [ ] All existing functionality preserved
- [ ] All existing tests pass
- [ ] No regressions
- [ ] Performance maintained or improved

### 8.3 Quality Criteria

- [ ] Code follows TypeScript strict mode
- [ ] No 'any' types used
- [ ] All functions documented
- [ ] Tests use Given-When-Then structure
- [ ] Hooks are reusable

---

## Appendices

### Appendix A: File Structure

```
src/
├── hooks/
│   ├── useSearch.ts
│   ├── useSearch.test.ts
│   ├── useHealthCheck.ts
│   ├── useHealthCheck.test.ts
│   ├── useDebounce.ts
│   ├── useDebounce.test.ts
│   └── index.ts
├── config/
│   ├── api.ts
│   └── constants.ts
└── ... (existing structure)
```

### Appendix B: Dependencies Graph

```
App.tsx
  ├─ useSearch ──┐
  ├─ useHealthCheck ──┐
  └─ useDebounce ──┐   │
                   │   │
                   ▼   ▼
            searchApi Service
                   │
                   ▼
            Backend API
```

---

**END OF MASTER SPECIFICATION**

**Document ID:** SPEC-MASTER-HOOKS-001  
**Version:** 1.0  
**Status:** DRAFT  
**Next Phase:** Test Generation (Phase 2)
