# useHealthCheck Hook Specification

**Project:** Haystack Search Engine - React Frontend  
**Component:** useHealthCheck Custom Hook  
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
5. [Polling Strategy](#5-polling-strategy)
6. [Error Handling](#6-error-handling)
7. [Implementation Details](#7-implementation-details)
8. [Test Requirements](#8-test-requirements)
9. [Microservices Considerations](#9-microservices-considerations)
10. [Cursor Implementation Notes](#10-cursor-implementation-notes)

---

## 1. Overview

### 1.1 Purpose

The `useHealthCheck` hook manages backend service health monitoring through periodic polling and provides health status to the UI. It ensures the frontend can detect backend availability and display appropriate status indicators.

### 1.2 Responsibilities

- Monitor backend health status
- Perform periodic health checks (every 5 seconds)
- Provide manual refresh capability
- Track last checked timestamp
- Handle health check errors gracefully
- Prevent memory leaks with proper cleanup
- Never throw exceptions (always graceful)

### 1.3 Non-Responsibilities

- UI rendering
- Backend health endpoint implementation
- Health status interpretation logic (delegated to searchApi)
- User notifications (handled by components)

---

## 2. Interface Definition

### 2.1 Hook Signature

```typescript
function useHealthCheck(): UseHealthCheckReturn
```

### 2.2 Return Type

```typescript
interface UseHealthCheckReturn {
  // State
  health: HealthStatus;        // 'healthy' | 'unhealthy' | 'unknown'
  lastChecked: Date | null;    // Timestamp of last check
  isRefreshing: boolean;       // True during manual refresh

  // Actions
  refreshHealth: () => Promise<void>;  // Manual refresh trigger
}
```

### 2.3 Type Definitions

```typescript
type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';
```

---

## 3. State Management

### 3.1 Initial State

```typescript
health: 'unknown'        // Unknown until first check
lastChecked: null        // No timestamp initially
isRefreshing: false      // Not refreshing initially
```

### 3.2 State Transitions

**Health State:**
- Initial: `'unknown'`
- After successful check (200 OK): `'healthy'`
- After failed check (any error): `'unhealthy'`
- After manual refresh: Updates based on check result

**LastChecked State:**
- Initial: `null`
- After any check (success or failure): `Date` object
- Never resets to null after first check

**IsRefreshing State:**
- Initial: `false`
- During manual refresh: `true`
- After manual refresh completes: `false`
- During auto-poll: `false` (not set to true)

---

## 4. Functional Requirements

### 4.1 Initialization Requirements

**FR-HEALTH-INIT-001:** SHALL initialize health as 'unknown'  
**FR-HEALTH-INIT-002:** SHALL initialize lastChecked as null  
**FR-HEALTH-INIT-003:** SHALL initialize isRefreshing as false  
**FR-HEALTH-INIT-004:** SHALL perform initial health check on mount  
**FR-HEALTH-INIT-005:** SHALL set up polling interval on mount  
**FR-HEALTH-INIT-006:** SHALL clear interval on unmount  

### 4.2 Health Check Execution Requirements

**FR-HEALTH-EXEC-001:** SHALL call `searchApi.checkHealth()`  
**FR-HEALTH-EXEC-002:** SHALL use 2-second timeout (faster than search)  
**FR-HEALTH-EXEC-003:** SHALL interpret 200 OK as 'healthy'  
**FR-HEALTH-EXEC-004:** SHALL interpret 503 as 'unhealthy'  
**FR-HEALTH-EXEC-005:** SHALL interpret network errors as 'unhealthy'  
**FR-HEALTH-EXEC-006:** SHALL interpret timeout errors as 'unhealthy'  
**FR-HEALTH-EXEC-007:** SHALL interpret any non-200 response as 'unhealthy'  
**FR-HEALTH-EXEC-008:** SHALL update lastChecked after each check  
**FR-HEALTH-EXEC-009:** SHALL NEVER throw exceptions  
**FR-HEALTH-EXEC-010:** SHALL handle all errors gracefully  

### 4.3 Polling Requirements

**FR-HEALTH-POLL-001:** SHALL poll health every 5 seconds  
**FR-HEALTH-POLL-002:** SHALL continue polling when unhealthy  
**FR-HEALTH-POLL-003:** SHALL continue polling when healthy  
**FR-HEALTH-POLL-004:** SHALL use setInterval for polling  
**FR-HEALTH-POLL-005:** SHALL clear interval on unmount  
**FR-HEALTH-POLL-006:** SHALL prevent state updates after unmount  
**FR-HEALTH-POLL-007:** SHALL NOT set isRefreshing during auto-polls  

### 4.4 Manual Refresh Requirements

**FR-HEALTH-REFRESH-001:** SHALL provide refreshHealth function  
**FR-HEALTH-REFRESH-002:** SHALL set isRefreshing to true before check  
**FR-HEALTH-REFRESH-003:** SHALL perform immediate health check  
**FR-HEALTH-REFRESH-004:** SHALL set isRefreshing to false after check  
**FR-HEALTH-REFRESH-005:** SHALL update health status  
**FR-HEALTH-REFRESH-006:** SHALL update lastChecked timestamp  
**FR-HEALTH-REFRESH-007:** SHALL handle errors gracefully  
**FR-HEALTH-REFRESH-008:** refreshHealth() called during auto-poll SHALL execute independently (both checks can be in flight simultaneously)  

---

## 5. Polling Strategy

### 5.1 Polling Interval

**Interval:** 5 seconds (5000 milliseconds)  
**Rationale:** 
- Frequent enough to detect outages quickly
- Not so frequent as to overload backend
- Balances responsiveness with resource usage

### 5.2 Polling Lifecycle

```
Component Mount
    │
    ▼
Initial Health Check (immediate)
    │
    ▼
Set up Interval (5s)
    │
    ├─ Poll 1 (after 5s)
    ├─ Poll 2 (after 10s)
    ├─ Poll 3 (after 15s)
    └─ ... (continues)
    │
    ▼
Component Unmount
    │
    ▼
Clear Interval
    │
    ▼
Prevent State Updates
```

### 5.3 Polling Behavior

**During Healthy State:**
- Continue polling every 5 seconds
- Update lastChecked on each poll
- Keep health as 'healthy'

**During Unhealthy State:**
- Continue polling every 5 seconds
- Update lastChecked on each poll
- Keep health as 'unhealthy'
- Detect recovery when backend becomes healthy

**During Manual Refresh:**
- Perform immediate check
- Set isRefreshing flag
- Don't interrupt auto-polling
- Update state after check completes

---

## 6. Error Handling

### 6.1 Error Types

**Network Errors:**
- Connection refused
- DNS resolution failure
- **Handling:** Set health to 'unhealthy', update lastChecked

**Timeout Errors:**
- Request exceeds 2-second timeout
- **Handling:** Set health to 'unhealthy', update lastChecked

**HTTP Errors:**
- 4xx/5xx responses
- **Handling:** Set health to 'unhealthy', update lastChecked

**Unknown Errors:**
- Any unexpected error
- **Handling:** Set health to 'unhealthy', update lastChecked

### 6.2 Error Handling Principles

**FR-HEALTH-ERROR-001:** SHALL catch all errors from checkHealth()  
**FR-HEALTH-ERROR-002:** SHALL NEVER throw exceptions  
**FR-HEALTH-ERROR-003:** SHALL always update lastChecked (even on error)  
**FR-HEALTH-ERROR-004:** SHALL set health to 'unhealthy' on any error  
**FR-HEALTH-ERROR-005:** SHALL continue polling after errors  
**FR-HEALTH-ERROR-006:** SHALL prevent state updates after unmount  

### 6.3 Cleanup and Memory Leaks

**FR-HEALTH-CLEANUP-001:** SHALL clear interval on unmount  
**FR-HEALTH-CLEANUP-002:** SHALL use cleanup function in useEffect  
**FR-HEALTH-CLEANUP-003:** SHALL check mounted state before updates  
**FR-HEALTH-CLEANUP-004:** SHALL prevent memory leaks  

---

## 7. Implementation Details

### 7.1 Hook Structure

```typescript
import { useState, useEffect, useCallback, useRef } from 'react';
import { checkHealth } from '../services/searchApi';
import type { HealthCheckResponse } from '../types/search';

type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';

const HEALTH_POLL_INTERVAL = 5000; // 5 seconds

export function useHealthCheck() {
  const [health, setHealth] = useState<HealthStatus>('unknown');
  const [lastChecked, setLastChecked] = useState<Date | null>(null);
  const [isRefreshing, setIsRefreshing] = useState<boolean>(false);
  const mountedRef = useRef<boolean>(true);

  // Perform health check (used by both polling and manual refresh)
  const performHealthCheck = useCallback(async (isManual: boolean = false) => {
    if (isManual) {
      setIsRefreshing(true);
    }

    try {
      const response: HealthCheckResponse = await checkHealth();
      
      // Only update state if component is still mounted
      if (mountedRef.current) {
        setHealth(response.status);
        setLastChecked(new Date());
      }
    } catch (err) {
      // checkHealth never throws, but handle just in case
      if (mountedRef.current) {
        setHealth('unhealthy');
        setLastChecked(new Date());
      }
    } finally {
      if (isManual && mountedRef.current) {
        setIsRefreshing(false);
      }
    }
  }, []);

  // Initial check and polling setup
  useEffect(() => {
    mountedRef.current = true;
    
    // Perform initial check
    performHealthCheck(false);

    // Set up polling interval
    const interval = setInterval(() => {
      if (mountedRef.current) {
        performHealthCheck(false);
      }
    }, HEALTH_POLL_INTERVAL);

    // Cleanup on unmount
    return () => {
      mountedRef.current = false;
      clearInterval(interval);
    };
  }, [performHealthCheck]);

  // Manual refresh function
  const refreshHealth = useCallback(async () => {
    await performHealthCheck(true);
  }, [performHealthCheck]);

  return {
    health,
    lastChecked,
    isRefreshing,
    refreshHealth,
  };
}
```

### 7.2 Dependencies

**External Dependencies:**
- `react` (useState, useEffect, useCallback, useRef)
- `../services/searchApi` (checkHealth function)
- `../types/search` (HealthCheckResponse type)

**Configuration:**
- `HEALTH_POLL_INTERVAL`: 5000ms (5 seconds)

### 7.3 Performance Considerations

**Polling Optimization:**
- Use `useRef` to track mounted state
- Prevent state updates after unmount
- Clear interval immediately on unmount

**Memory Management:**
- Cleanup function in useEffect
- Clear interval on unmount
- Prevent memory leaks

---

## 8. Test Requirements

### 8.1 Test Coverage Requirements

**TR-HEALTH-001:** Test initial state values  
**TR-HEALTH-002:** Test initial health check on mount  
**TR-HEALTH-003:** Test polling interval setup  
**TR-HEALTH-004:** Test polling every 5 seconds  
**TR-HEALTH-005:** Test health status updates on successful check  
**TR-HEALTH-006:** Test health status updates on failed check  
**TR-HEALTH-007:** Test lastChecked updates after each check  
**TR-HEALTH-008:** Test manual refresh sets isRefreshing  
**TR-HEALTH-009:** Test manual refresh performs immediate check  
**TR-HEALTH-010:** Test manual refresh clears isRefreshing after completion  
**TR-HEALTH-011:** Test auto-poll does not set isRefreshing  
**TR-HEALTH-012:** Test interval cleared on unmount  
**TR-HEALTH-013:** Test state updates prevented after unmount  
**TR-HEALTH-014:** Test continues polling when unhealthy  
**TR-HEALTH-015:** Test detects recovery (unhealthy → healthy)  
**TR-HEALTH-016:** Test handles network errors gracefully  
**TR-HEALTH-017:** Test handles timeout errors gracefully  
**TR-HEALTH-018:** Test handles HTTP errors gracefully  
**TR-HEALTH-019:** Test never throws exceptions  
**TR-HEALTH-020:** Test multiple rapid manual refreshes  

### 8.2 Test Structure

**Given-When-Then Format:**
- Given: Component mounted or specific state
- When: Health check performed or time advances
- Then: Expected state changes

**Mock Requirements:**
- Mock `searchApi.checkHealth()` function
- Mock different response scenarios
- Use fake timers for polling tests

**Test Isolation:**
- Each test is independent
- Reset timers between tests
- Clean up intervals

---

## 9. Microservices Considerations

### 9.1 Stateless Design

**MS-HEALTH-001:** Hook maintains no server-side state  
**MS-HEALTH-002:** Each health check is independent request  
**MS-HEALTH-003:** No session management required  
**MS-HEALTH-004:** Works with load-balanced backends  

### 9.2 Health Check Strategy

**MS-HEALTH-005:** Polls backend health endpoint  
**MS-HEALTH-006:** Detects outages within 5 seconds  
**MS-HEALTH-007:** Continues monitoring during outages  
**MS-HEALTH-008:** Detects recovery automatically  

### 9.3 Scalability

**MS-HEALTH-009:** Multiple frontend instances poll independently  
**MS-HEALTH-010:** No coordination between instances needed  
**MS-HEALTH-011:** Backend can handle multiple health checks  
**MS-HEALTH-012:** Supports horizontal scaling  

### 9.4 Failure Handling

**MS-HEALTH-013:** Frontend remains functional when backend down  
**MS-HEALTH-014:** Health status displayed to users  
**MS-HEALTH-015:** No cascading failures  
**MS-HEALTH-016:** Graceful degradation  

**Optional Future Enhancement:**
- Consider exponential backoff if backend unhealthy for >5 minutes
- Example: After 5 minutes unhealthy, poll every 10 seconds instead of 5
- Not required for initial implementation

---

## 10. Cursor Implementation Notes

When generating code from this specification:
- Use exact type signatures from Section 2.2 and 2.3
- Follow implementation structure from Section 7.1
- Implement all polling behavior from Section 5
- Include all test cases from Section 8
- Use useRef for mounted state tracking
- Use useCallback for performHealthCheck and refreshHealth
- Include JSDoc comments for the hook

---

## Appendices

### Appendix A: Usage Example

```typescript
import { useHealthCheck } from '../hooks/useHealthCheck';

function MyComponent() {
  const {
    health,
    lastChecked,
    isRefreshing,
    refreshHealth,
  } = useHealthCheck();

  return (
    <div>
      <div>Status: {health}</div>
      <div>Last checked: {lastChecked?.toLocaleTimeString() || 'Never'}</div>
      <button onClick={refreshHealth} disabled={isRefreshing}>
        {isRefreshing ? 'Refreshing...' : 'Refresh'}
      </button>
    </div>
  );
}
```

### Appendix B: Health Status Interpretation

| Backend Response | Health Status |
|------------------|---------------|
| 200 OK with "OK" body | 'healthy' |
| 503 Service Unavailable | 'unhealthy' |
| Network error | 'unhealthy' |
| Timeout error | 'unhealthy' |
| Any other error | 'unhealthy' |
| Not checked yet | 'unknown' |

### Appendix C: Polling Timeline Example

```
Time  | Action                    | Health  | lastChecked
------|---------------------------|---------|------------
0s    | Component mounts          | unknown | null
0s    | Initial check starts      | unknown | null
0.5s  | Initial check completes   | healthy | 00:00:00.5
5s    | Poll 1 starts             | healthy | 00:00:00.5
5.5s  | Poll 1 completes          | healthy | 00:00:05.5
10s   | Poll 2 starts             | healthy | 00:00:05.5
10.5s | Poll 2 completes          | healthy | 00:00:10.5
15s   | Backend goes down          | healthy | 00:00:10.5
15s   | Poll 3 starts             | healthy | 00:00:10.5
15.2s | Poll 3 fails               | unhealthy| 00:00:15.2
20s   | Poll 4 starts             | unhealthy| 00:00:15.2
20.2s | Poll 4 fails               | unhealthy| 00:00:20.2
25s   | Backend recovers           | unhealthy| 00:00:20.2
25s   | Poll 5 starts              | unhealthy| 00:00:20.2
25.5s | Poll 5 succeeds            | healthy | 00:00:25.5
```

### Appendix D: Implementation Review Checklist

Before marking useHealthCheck as complete:
- [ ] All FR-HEALTH requirements implemented (Section 4)
- [ ] All 20 test cases pass (Section 8)
- [ ] Zero TypeScript errors
- [ ] Polling interval is exactly 5000ms
- [ ] Cleanup prevents memory leaks
- [ ] Code follows structure from Section 7.1
- [ ] useRef prevents post-unmount updates
- [ ] JSDoc comments present

---

**END OF SPECIFICATION**

**Document ID:** SPEC-USE-HEALTH-CHECK-001  
**Version:** 1.0  
**Status:** DRAFT  
**Next Document:** useDebounce_SPEC.md
