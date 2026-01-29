# Phase 2 GUI - Custom Hooks Requirements

**Project:** Haystack Search Engine - React Frontend  
**Phase:** Phase 2 GUI - Custom Hooks Refactoring  
**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Requirements Definition - Pending Approval  
**Author:** Technical Requirements Team  

---

## Document Control

| Version | Date | Author | Status | Changes |
|---------|------|--------|--------|---------|
| 1.0 | 2026-01-28 | Technical Team | Draft | Initial requirements |

**Approval Required From:**
- [ ] Vilas Tadoori (Technical Lead)
- [ ] Development Team
- [ ] QA Team

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Background & Context](#2-background--context)
3. [Stakeholders](#3-stakeholders)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Technical Requirements](#6-technical-requirements)
7. [Folder Structure Requirements](#7-folder-structure-requirements)
8. [Acceptance Criteria](#8-acceptance-criteria)
9. [Constraints & Dependencies](#9-constraints--dependencies)
10. [Risks & Mitigation](#10-risks--mitigation)
11. [Success Metrics](#11-success-metrics)
12. [Timeline & Phases](#12-timeline--phases)
13. [Next Steps](#13-next-steps)

---

## 1. Executive Summary

### 1.1 Purpose
Extract business logic from App.tsx into three reusable custom React hooks to improve code maintainability, testability, and align with microservices architecture principles.

### 1.2 Scope

**In Scope:**
- Create `useSearch` hook for search state management
- Create `useHealthCheck` hook for backend health monitoring
- Create `useDebounce` hook for input debouncing
- Refactor App.tsx to use these hooks
- Maintain 100% backward compatibility
- Create comprehensive documentation structure

**Out of Scope:**
- Changes to backend C++ service
- New UI features or components
- Changes to existing component behavior
- Database or backend modifications

### 1.3 Business Value

| Benefit | Current | Target | Impact |
|---------|---------|--------|--------|
| **Maintainability** | App.tsx 150 lines | App.tsx <60 lines | 60% reduction |
| **Testability** | Coupled to UI | Independent testing | Faster test execution |
| **Reusability** | Copy-paste logic | Import hook | DRY principle |
| **Scalability** | Monolithic | Microservices-ready | Cloud deployment |

---

## 2. Background & Context

### 2.1 Current Architecture

```
┌────────────────────────────────────────┐
│  App.tsx (Monolithic Component)        │
│  ~150 lines                            │
│                                        │
│  ├─ Search State (query, results)     │
│  ├─ Health State (status, polling)    │
│  ├─ API Calls (axios)                 │
│  ├─ Error Handling                    │
│  ├─ Loading States                    │
│  └─ UI Rendering (JSX)                │
└────────────────────────────────────────┘
         │
         ↓ HTTP REST API
┌────────────────────────────────────────┐
│  C++ Search Backend (searchd)          │
│  Port: 8900                            │
│  Endpoints: /search, /health           │
└────────────────────────────────────────┘
```

**Problems:**
- Too many responsibilities in one component
- Cannot test logic independently from UI
- Cannot reuse search logic in other components
- Violates Single Responsibility Principle
- Not microservices-friendly

### 2.2 Target Architecture

```
┌────────────────────────────────────────┐
│  App.tsx (Clean Component)             │
│  ~50 lines - UI Only                   │
│                                        │
│  Uses:                                 │
│  ├─ useSearch()      ─────┐           │
│  ├─ useHealthCheck() ─────┤           │
│  └─ useDebounce()    ─────┤           │
└───────────────────────────┼───────────┘
                            │
        ┌───────────────────┴────────────────┐
        │                                    │
┌───────▼─────────┐  ┌──────────────────────▼─┐
│  Custom Hooks   │  │  Reusable Anywhere     │
│  (Business      │  │  - Other components    │
│   Logic)        │  │  - Future features     │
│                 │  │  - Test in isolation   │
└────────┬────────┘  └────────────────────────┘
         │
         ↓ HTTP REST API
┌────────────────────────────────────────┐
│  C++ Search Backend (searchd)          │
└────────────────────────────────────────┘
```

**Benefits:**
- Clear separation of concerns
- Independent testing
- Reusable across components
- Microservices-compatible
- Easier to maintain

---

## 3. Stakeholders

| Role | Name | Responsibility | Contact |
|------|------|---------------|---------|
| **Technical Lead** | Vilas Tadoori | Final approval, architecture decisions | - |
| **Frontend Dev** | Development Team | Implementation, testing | - |
| **QA Engineer** | QA Team | Test validation, regression testing | - |
| **DevOps** | Ops Team | Deployment, monitoring | - |
| **End Users** | Search Users | Use the interface | - |

---

## 4. Functional Requirements

### 4.1 useSearch Hook

**REQ-SEARCH-001: Search State Management**

**Priority:** High  
**Type:** Functional

**Description:**  
Manage all search-related state including query, results, metrics, errors, and loading states.

**Interface:**
```typescript
function useSearch(): {
  query: string;
  limit: number;
  results: SearchResult[];
  metrics: SearchMetrics | null;
  error: string | null;
  isLoading: boolean;
  setQuery: (query: string) => void;
  setLimit: (limit: number) => void;
  executeSearch: () => Promise<void>;
  clearSearch: () => void;
}
```

**Functional Requirements:**

**FR-SEARCH-001:** SHALL maintain query as string
**FR-SEARCH-002:** SHALL maintain limit as number (1-50)
**FR-SEARCH-003:** SHALL maintain results as SearchResult array
**FR-SEARCH-004:** SHALL maintain metrics (query, count, latency)
**FR-SEARCH-005:** SHALL maintain error as nullable string
**FR-SEARCH-006:** SHALL maintain isLoading boolean

**FR-SEARCH-010:** SHALL provide setQuery function
**FR-SEARCH-011:** SHALL provide setLimit function (validate 1-50)
**FR-SEARCH-012:** SHALL validate query non-empty before search
**FR-SEARCH-013:** SHALL trim whitespace from query

**FR-SEARCH-020:** SHALL call backend GET /search?q={query}&k={limit}
**FR-SEARCH-021:** SHALL track API latency (start to end time)
**FR-SEARCH-022:** SHALL update results on success
**FR-SEARCH-023:** SHALL update metrics on success
**FR-SEARCH-024:** SHALL clear previous results before new search
**FR-SEARCH-025:** SHALL set error message on failure
**FR-SEARCH-026:** SHALL set isLoading during API call

**FR-SEARCH-030:** SHALL handle NetworkError gracefully
**FR-SEARCH-031:** SHALL handle TimeoutError gracefully
**FR-SEARCH-032:** SHALL handle HttpError gracefully
**FR-SEARCH-033:** SHALL handle InvalidResponseError gracefully
**FR-SEARCH-034:** SHALL provide user-friendly error messages
**FR-SEARCH-035:** SHALL NOT throw exceptions (graceful degradation)

**FR-SEARCH-040:** SHALL provide clearSearch function
**FR-SEARCH-041:** SHALL reset all state on clear

**FR-SEARCH-050:** SHALL be stateless (no server state)
**FR-SEARCH-051:** SHALL make independent API calls
**FR-SEARCH-052:** SHALL work with load-balanced frontends

---

### 4.2 useHealthCheck Hook

**REQ-HEALTH-001: Backend Health Monitoring**

**Priority:** High  
**Type:** Functional

**Description:**  
Monitor backend service health through periodic polling and expose health status to UI.

**Interface:**
```typescript
function useHealthCheck(): {
  health: HealthStatus; // 'healthy' | 'unhealthy' | 'unknown'
  lastChecked: Date | null;
  isRefreshing: boolean;
  refreshHealth: () => Promise<void>;
}
```

**Functional Requirements:**

**FR-HEALTH-001:** SHALL maintain health status (healthy/unhealthy/unknown)
**FR-HEALTH-002:** SHALL initialize with 'unknown' state
**FR-HEALTH-003:** SHALL maintain lastChecked timestamp
**FR-HEALTH-004:** SHALL maintain isRefreshing boolean

**FR-HEALTH-010:** SHALL call backend GET /health endpoint
**FR-HEALTH-011:** SHALL interpret 200 OK as 'healthy'
**FR-HEALTH-012:** SHALL interpret 503 as 'unhealthy'
**FR-HEALTH-013:** SHALL interpret network errors as 'unhealthy'
**FR-HEALTH-014:** SHALL use 2-second timeout (faster than search)
**FR-HEALTH-015:** SHALL update lastChecked after each check
**FR-HEALTH-016:** SHALL NEVER throw exceptions

**FR-HEALTH-020:** SHALL check health on component mount
**FR-HEALTH-021:** SHALL poll health every 5 seconds
**FR-HEALTH-022:** SHALL continue polling when unhealthy
**FR-HEALTH-023:** SHALL clear interval on unmount
**FR-HEALTH-024:** SHALL prevent state updates after unmount

**FR-HEALTH-030:** SHALL provide refreshHealth function
**FR-HEALTH-031:** SHALL set isRefreshing during manual refresh
**FR-HEALTH-032:** SHALL NOT set isRefreshing for auto-polls
**FR-HEALTH-033:** SHALL trigger immediate check on manual refresh

---

### 4.3 useDebounce Hook

**REQ-DEBOUNCE-001: Value Debouncing**

**Priority:** Medium  
**Type:** Functional

**Description:**  
Generic debounce utility to delay rapidly changing values and reduce API load.

**Interface:**
```typescript
function useDebounce<T>(value: T, delay: number): T
```

**Functional Requirements:**

**FR-DEBOUNCE-001:** SHALL accept any value type (generic T)
**FR-DEBOUNCE-002:** SHALL accept delay in milliseconds
**FR-DEBOUNCE-003:** SHALL return debounced value of same type
**FR-DEBOUNCE-004:** SHALL return initial value immediately

**FR-DEBOUNCE-010:** SHALL delay returning updated value by delay
**FR-DEBOUNCE-011:** SHALL reset timer on each value change
**FR-DEBOUNCE-012:** SHALL return latest value after delay expires
**FR-DEBOUNCE-013:** SHALL return only final value after rapid changes

**FR-DEBOUNCE-020:** SHALL clear timeout on unmount
**FR-DEBOUNCE-021:** SHALL prevent memory leaks
**FR-DEBOUNCE-022:** SHALL handle multiple rapid updates

**FR-DEBOUNCE-030:** SHALL support different delays (100-1000ms)
**FR-DEBOUNCE-031:** SHALL support different types (string, number, object)

---

## 5. Non-Functional Requirements

### 5.1 Performance

**NFR-PERF-001:** Search API calls SHALL complete within 5 seconds  
**NFR-PERF-002:** Health checks SHALL complete within 2 seconds  
**NFR-PERF-003:** Debouncing SHALL reduce API calls by >70%  
**NFR-PERF-004:** Hooks SHALL not cause memory leaks  
**NFR-PERF-005:** Hooks SHALL not cause unnecessary re-renders  

### 5.2 Reliability

**NFR-REL-001:** Hooks SHALL handle 100% of expected errors  
**NFR-REL-002:** API failures SHALL NOT crash application  
**NFR-REL-003:** Frontend SHALL remain functional when backend down  
**NFR-REL-004:** Health checks SHALL detect outages within 5 seconds  
**NFR-REL-005:** Error messages SHALL be user-friendly  

### 5.3 Maintainability

**NFR-MAINT-001:** Each hook file SHALL be <100 lines  
**NFR-MAINT-002:** Code SHALL follow TypeScript strict mode  
**NFR-MAINT-003:** No 'any' types SHALL be used  
**NFR-MAINT-004:** All functions SHALL be typed  
**NFR-MAINT-005:** All hooks SHALL have JSDoc comments  

### 5.4 Testability

**NFR-TEST-001:** Each hook SHALL be testable independently  
**NFR-TEST-002:** Test coverage SHALL be >95% per hook  
**NFR-TEST-003:** All edge cases SHALL have test coverage  
**NFR-TEST-004:** Tests SHALL use Given-When-Then structure  
**NFR-TEST-005:** Tests SHALL explain microservices implications  

### 5.5 Scalability

**NFR-SCALE-001:** SHALL support multiple frontend instances  
**NFR-SCALE-002:** SHALL support backend horizontal scaling  
**NFR-SCALE-003:** SHALL be stateless (no server sessions)  
**NFR-SCALE-004:** SHALL use REST API only  
**NFR-SCALE-005:** SHALL be independently deployable  

### 5.6 Security

**NFR-SEC-001:** Query input SHALL be validated client-side  
**NFR-SEC-002:** Limit parameter SHALL be bounded (1-50)  
**NFR-SEC-003:** SHALL support HTTPS  
**NFR-SEC-004:** API endpoints SHALL be environment-configurable  
**NFR-SEC-005:** No sensitive data SHALL be logged  

---

## 6. Technical Requirements

### 6.1 Technology Stack

**TECH-STACK-001:** React 18.3+  
**TECH-STACK-002:** TypeScript 5.6+ (strict mode)  
**TECH-STACK-003:** Vite 7+ (build tool)  
**TECH-STACK-004:** Vitest 2.1+ (testing)  
**TECH-STACK-005:** React Testing Library 16+  
**TECH-STACK-006:** Axios 1.7+ (HTTP client)  

### 6.2 Code Standards

**TECH-STD-001:** TypeScript strict mode enabled  
**TECH-STD-002:** No 'any' types allowed  
**TECH-STD-003:** All functions must have return types  
**TECH-STD-004:** All parameters must be typed  
**TECH-STD-005:** Functional components only  
**TECH-STD-006:** Hooks follow "use" naming convention  
**TECH-STD-007:** Proper dependency arrays in useEffect  
**TECH-STD-008:** Cleanup functions for all side effects  

### 6.3 Testing Standards

**TECH-TEST-001:** TDD approach (tests before implementation)  
**TECH-TEST-002:** Given-When-Then test structure  
**TECH-TEST-003:** Comprehensive documentation per test  
**TECH-TEST-004:** Mock external dependencies  
**TECH-TEST-005:** Test coverage >95%  

### 6.4 Microservices Standards

**TECH-MICRO-001:** Stateless design (no server sessions)  
**TECH-MICRO-002:** REST API communication only  
**TECH-MICRO-003:** Independent service deployment  
**TECH-MICRO-004:** Graceful failure handling  
**TECH-MICRO-005:** Health check endpoints  
**TECH-MICRO-006:** Environment-based configuration  

---

## 7. Folder Structure Requirements

### 7.1 Required Folder Structure

**REQ-FOLDER-001:** The project SHALL have this documentation structure:

```
/Users/vilastadoori/_Haystack_proj/gui/haystack-gui/
├── docs/
│   ├── requirements/
│   │   ├── PHASE_2_GUI_HOOKS_REQUIREMENTS.md (this document)
│   │   ├── MICROSERVICES_ARCHITECTURE.md
│   │   └── API_CONTRACT.md
│   └── specs/
│       ├── PHASE_2_GUI_HOOKS_SPECIFICATION.md (master spec)
│       ├── useSearch_SPEC.md
│       ├── useHealthCheck_SPEC.md
│       └── useDebounce_SPEC.md
```

**REQ-FOLDER-002:** Source code structure:

```
src/
├── hooks/
│   ├── useSearch.ts
│   ├── useSearch.test.ts
│   ├── useHealthCheck.ts
│   ├── useHealthCheck.test.ts
│   ├── useDebounce.ts
│   ├── useDebounce.test.ts
│   └── index.ts (barrel export)
├── config/
│   ├── api.ts (API endpoints, timeouts)
│   └── constants.ts (polling intervals, limits)
└── ... (existing structure)
```

---

## 8. Acceptance Criteria

### 8.1 Documentation Criteria

**AC-DOC-001:** ✅ Requirements document approved  
**AC-DOC-002:** ✅ Microservices architecture document created  
**AC-DOC-003:** ✅ Three detailed spec documents created  
**AC-DOC-004:** ✅ All specs reviewed and approved  
**AC-DOC-005:** ✅ Folder structure created  

### 8.2 Implementation Criteria

**AC-IMPL-001:** ✅ useSearch hook implements all FR-SEARCH requirements  
**AC-IMPL-002:** ✅ useHealthCheck hook implements all FR-HEALTH requirements  
**AC-IMPL-003:** ✅ useDebounce hook implements all FR-DEBOUNCE requirements  
**AC-IMPL-004:** ✅ All 38 hook tests pass  
**AC-IMPL-005:** ✅ Test coverage >95% for each hook  

### 8.3 Refactoring Criteria

**AC-REFACTOR-001:** ✅ App.tsx reduced from ~150 to <60 lines  
**AC-REFACTOR-002:** ✅ All logic moved to hooks  
**AC-REFACTOR-003:** ✅ All existing tests still pass  
**AC-REFACTOR-004:** ✅ No functional changes  
**AC-REFACTOR-005:** ✅ No visual changes  

### 8.4 Quality Criteria

**AC-QUALITY-001:** ✅ Zero TypeScript errors  
**AC-QUALITY-002:** ✅ Zero ESLint errors  
**AC-QUALITY-003:** ✅ No 'any' types used  
**AC-QUALITY-004:** ✅ All functions documented  
**AC-QUALITY-005:** ✅ README updated  

---

## 9. Constraints & Dependencies

### 9.1 Technical Constraints

**CONST-001:** Must use React 18+ (hooks API)  
**CONST-002:** Cannot modify backend C++ service  
**CONST-003:** Must use existing /search and /health endpoints  
**CONST-004:** Cannot change request/response formats  
**CONST-005:** Must support browsers with ES2020  

### 9.2 Dependencies

**DEP-001:** Axios (already installed)  
**DEP-002:** Vitest (already installed)  
**DEP-003:** React Testing Library (already installed)  
**DEP-004:** C++ searchd running on port 8900  
**DEP-005:** Existing components must not break  

---

## 10. Risks & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Breaking existing functionality | Medium | High | Comprehensive tests, incremental refactoring |
| Performance degradation | Low | Medium | Performance testing before/after |
| Memory leaks | Medium | High | Thorough cleanup testing |
| Schedule overrun | Low | Medium | Clear specs, buffer time |

---

## 11. Success Metrics

| Metric | Current | Target | Measurement |
|--------|---------|--------|-------------|
| App.tsx lines | 150 | <60 | Line count |
| Test coverage | N/A | >95% | npm test --coverage |
| TypeScript errors | 0 | 0 | npx tsc --noEmit |
| API call reduction | N/A | >70% | Network monitoring |
| Test pass rate | 100% | 100% | CI/CD |

---

## 12. Timeline & Phases

### Phase Breakdown

**Phase 1: Documentation** (2 hours) - THIS PHASE
- Requirements document ✅
- Architecture document (next)
- Spec documents (next)
- **Approval gate**

**Phase 2: Test Generation** (2-3 hours)
- Generate hook tests with Cursor
- Verify all tests fail (TDD Red)

**Phase 3: Implementation** (3-4 hours)
- Implement hooks with Cursor
- Verify all tests pass (TDD Green)

**Phase 4: Refactoring** (1-2 hours)
- Refactor App.tsx
- Verify no regressions

**Phase 5: Documentation** (1 hour)
- Update README
- Finalize documentation

**Total:** 9-12 hours (1.5-2 days)

---

## 13. Next Steps

### After This Document is Approved:

1. **Create MICROSERVICES_ARCHITECTURE.md**
   - Service boundaries
   - API contracts
   - Deployment architecture
   - Scaling strategy

2. **Create Detailed Specs**
   - `useSearch_SPEC.md`
   - `useHealthCheck_SPEC.md`
   - `useDebounce_SPEC.md`

3. **Create Folder Structure**
   ```bash
   mkdir -p docs/requirements docs/specs src/config
   ```

4. **Begin Implementation**
   - Generate tests
   - Implement hooks
   - Refactor App.tsx

---

## Appendices

### Appendix A: Glossary

- **Hook:** React function starting with "use" for state/effects
- **Microservices:** Independent, deployable services
- **TDD:** Test-Driven Development (tests first)
- **Debounce:** Delay execution until input stops changing
- **Graceful Degradation:** Continue functioning when dependencies fail

### Appendix B: References

- React Hooks: https://react.dev/reference/react/hooks
- Microservices Patterns: https://microservices.io/patterns/
- Testing Library: https://testing-library.com/

---

## Approval Sign-Off

**This requirements document requires approval before proceeding to specifications.**

| Approver | Role | Status | Date | Signature |
|----------|------|--------|------|-----------|
| Vilas Tadoori | Technical Lead | ⏸️ Pending | - | - |
| Development Team | Implementation | ⏸️ Pending | - | - |
| QA Team | Testing | ⏸️ Pending | - | - |

**Approved:** ☐ Yes ☐ No  
**Date:** _____________  
**Comments:** _______________________________________________

---

**END OF REQUIREMENTS DOCUMENT**

**Document ID:** REQ-PHASE2-GUI-HOOKS-001  
**Version:** 1.0  
**Status:** PENDING APPROVAL  
**Next Document:** MICROSERVICES_ARCHITECTURE.md
