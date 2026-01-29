# Microservices Architecture - Haystack Search Engine GUI

**Project:** Haystack Search Engine - React Frontend  
**Document Type:** Architecture Specification  
**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Draft  
**Author:** Technical Architecture Team  

---

## Document Control

| Version | Date | Author | Status | Changes |
|---------|------|--------|--------|---------|
| 1.0 | 2026-01-28 | Architecture Team | Draft | Initial architecture document |

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Service Boundaries](#3-service-boundaries)
4. [API Contracts](#4-api-contracts)
5. [Deployment Architecture](#5-deployment-architecture)
6. [Scaling Strategy](#6-scaling-strategy)
7. [Data Flow](#7-data-flow)
8. [Error Handling Strategy](#8-error-handling-strategy)
9. [Security Architecture](#9-security-architecture)
10. [Monitoring & Observability](#10-monitoring--observability)

---

## 1. Executive Summary

### 1.1 Purpose

This document defines the microservices architecture for the Haystack Search Engine GUI, focusing on how the React frontend interacts with backend services and how the architecture supports independent scaling, deployment, and failure isolation.

### 1.2 Key Principles

- **Stateless Design:** Frontend maintains no server-side sessions
- **Independent Deployment:** Frontend and backend can be deployed separately
- **Horizontal Scaling:** Multiple frontend instances can run simultaneously
- **Graceful Degradation:** Frontend remains functional when backend is unavailable
- **REST API Communication:** Standard HTTP/REST for all service communication

### 1.3 Architecture Goals

| Goal | Description | Benefit |
|------|-------------|---------|
| **Scalability** | Support multiple frontend instances | Handle increased load |
| **Resilience** | Frontend works when backend down | Better user experience |
| **Maintainability** | Clear service boundaries | Easier to update |
| **Testability** | Independent testing | Faster development |
| **Deployment** | Independent releases | Reduced risk |

---

## 2. Architecture Overview

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Frontend Layer                            │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │   Browser    │  │   Browser    │  │   Browser    │    │
│  │  Instance 1  │  │  Instance 2  │  │  Instance N  │    │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘    │
│         │                  │                  │             │
│         └──────────────────┴──────────────────┘            │
│                            │                                 │
│                   ┌────────▼────────┐                        │
│                   │  React App     │                        │
│                   │  (Stateless)  │                        │
│                   └────────┬───────┘                        │
│                            │                                 │
│         ┌──────────────────┼──────────────────┐            │
│         │                  │                  │             │
│  ┌──────▼──────┐  ┌───────▼──────┐  ┌───────▼──────┐    │
│  │ useSearch   │  │useHealthCheck │  │ useDebounce  │    │
│  │   Hook      │  │     Hook      │  │    Hook      │    │
│  └──────┬──────┘  └───────┬───────┘  └──────────────┘    │
│         │                  │                                 │
└─────────┼──────────────────┼─────────────────────────────────┘
          │                  │
          │ HTTP REST API    │
          │                  │
┌─────────▼──────────────────▼─────────────────────────────────┐
│                    Backend Layer                              │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  Backend      │  │  Backend      │  │  Backend      │     │
│  │  Instance 1   │  │  Instance 2   │  │  Instance N   │     │
│  │  (searchd)    │  │  (searchd)    │  │  (searchd)    │     │
│  │  Port: 8900   │  │  Port: 8900   │  │  Port: 8900   │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│         │                  │                  │              │
│         └──────────────────┴──────────────────┘              │
│                            │                                 │
│                   ┌────────▼────────┐                        │
│                   │  Load Balancer  │                        │
│                   │  (Optional)    │                        │
│                   └─────────────────┘                        │
└───────────────────────────────────────────────────────────────┘
```

### 2.2 Component Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    React Application                         │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │              App.tsx (Orchestrator)                 │    │
│  │  - Composes hooks                                   │    │
│  │  - Renders UI components                            │    │
│  │  - Handles user interactions                        │    │
│  └───────────────┬────────────────────────────────────┘    │
│                  │                                          │
│    ┌─────────────┼─────────────┐                           │
│    │             │             │                           │
│ ┌──▼──────┐ ┌───▼──────┐ ┌───▼──────┐                    │
│ │useSearch│ │useHealth │ │useDebounce│                    │
│ │  Hook   │ │  Check   │ │   Hook   │                    │
│ └──┬──────┘ └───┬──────┘ └──────────┘                    │
│    │             │                                         │
│    └─────────────┴─────────────┐                           │
│                                │                           │
│                    ┌───────────▼───────────┐              │
│                    │   searchApi Service   │              │
│                    │   - search()          │              │
│                    │   - checkHealth()     │              │
│                    └───────────┬───────────┘              │
│                                │                           │
└────────────────────────────────┼───────────────────────────┘
                                 │
                        HTTP REST API
                                 │
┌────────────────────────────────▼───────────────────────────┐
│              C++ Backend Service (searchd)                 │
│              - GET /search?q={query}&k={limit}            │
│              - GET /health                                  │
└────────────────────────────────────────────────────────────┘
```

---

## 3. Service Boundaries

### 3.1 Frontend Service (React Application)

**Responsibilities:**
- User interface rendering
- User interaction handling
- Client-side state management
- API communication
- Error display and user feedback

**Non-Responsibilities:**
- Business logic (moved to hooks)
- Data persistence
- Authentication/authorization (future)
- Server-side rendering
- Database access

**Deployment:**
- Static files (HTML, CSS, JS)
- Served via CDN or web server
- No server-side runtime required
- Can be deployed to multiple regions

**Scaling:**
- Horizontal scaling (multiple instances)
- CDN distribution
- No shared state between instances
- Stateless design

### 3.2 Backend Service (C++ searchd)

**Responsibilities:**
- Search query processing
- Document indexing and retrieval
- Health status reporting
- API endpoint handling

**Non-Responsibilities:**
- UI rendering
- Client-side state
- Frontend routing
- User session management

**Deployment:**
- C++ binary service
- Runs on port 8900
- Can be containerized
- Can be load-balanced

**Scaling:**
- Horizontal scaling (multiple instances)
- Load balancer distribution
- Stateless API design
- Independent scaling from frontend

### 3.3 Service Communication

**Protocol:** HTTP/REST  
**Format:** JSON  
**Authentication:** None (current), HTTPS recommended  
**Timeout:** 
- Search: 5 seconds
- Health: 2 seconds

**Stateless:** Each request is independent, no sessions

---

## 4. API Contracts

### 4.1 Search API Contract

**Endpoint:** `GET /search`

**Request:**
```
GET /search?q={query}&k={limit}
```

**Query Parameters:**
- `q` (string, required): Search query (URL encoded)
- `k` (number, required): Number of results (1-50)

**Response (Success - 200 OK):**
```json
{
  "query": "migration",
  "results": [
    {
      "docId": 1,
      "score": 0.95,
      "snippet": "Document snippet text..."
    }
  ]
}
```

**Response (Error - 4xx/5xx):**
- 400: Bad Request (invalid parameters)
- 500: Internal Server Error
- 503: Service Unavailable

**Error Handling:**
- Frontend catches all errors
- Displays user-friendly messages
- Does not crash application

### 4.2 Health API Contract

**Endpoint:** `GET /health`

**Request:**
```
GET /health
```

**Response (Healthy - 200 OK):**
```
OK
```
(Plain text response, case-insensitive)

**Response (Unhealthy - 503):**
```
503 Service Unavailable
```

**Response (Error):**
- Network errors → Frontend interprets as 'unhealthy'
- Timeout errors → Frontend interprets as 'unhealthy'
- Any non-200 response → Frontend interprets as 'unhealthy'

**Polling Strategy:**
- Initial check on mount
- Poll every 5 seconds
- Continue polling when unhealthy
- Manual refresh available

---

## 5. Deployment Architecture

### 5.1 Frontend Deployment

**Build Process:**
```bash
npm run build
# Output: dist/ directory with static files
```

**Deployment Options:**

**Option 1: Static Hosting (CDN)**
```
┌─────────────┐
│   CDN       │
│  (CloudFlare │
│   / AWS)    │
└──────┬──────┘
       │
  ┌────▼────┐
  │ Browser │
  └─────────┘
```

**Option 2: Web Server**
```
┌─────────────┐
│ Nginx/      │
│ Apache      │
└──────┬──────┘
       │
  ┌────▼────┐
  │ Browser │
  └─────────┘
```

**Option 3: Container (Docker)**
```dockerfile
FROM nginx:alpine
COPY dist/ /usr/share/nginx/html/
EXPOSE 80
```

**Scaling:**
- Deploy to multiple regions
- Use CDN for global distribution
- No coordination needed between instances
- Stateless design enables easy scaling

### 5.2 Backend Deployment

**Deployment Options:**

**Option 1: Direct Binary**
```bash
./searchd --port 8900
```

**Option 2: Container (Docker)**
```dockerfile
FROM ubuntu:22.04
COPY searchd /usr/local/bin/
EXPOSE 8900
CMD ["searchd", "--port", "8900"]
```

**Option 3: Kubernetes**
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: searchd
spec:
  replicas: 3
  ...
```

**Scaling:**
- Horizontal scaling (multiple instances)
- Load balancer in front
- Stateless API enables easy scaling
- Independent from frontend scaling

### 5.3 Environment Configuration

**Frontend Configuration (`src/config/api.ts`):**
```typescript
export const API_CONFIG = {
  BASE_URL: import.meta.env.VITE_API_BASE_URL || 'http://localhost:8900',
  SEARCH_TIMEOUT: 5000,
  HEALTH_TIMEOUT: 2000,
  HEALTH_POLL_INTERVAL: 5000,
};
```

**Environment Variables:**
- `VITE_API_BASE_URL`: Backend API URL (default: http://localhost:8900)
- Supports different environments (dev, staging, prod)

---

## 6. Scaling Strategy

### 6.1 Frontend Scaling

**Current Capacity:** Single instance  
**Target Capacity:** Unlimited (CDN distributed)

**Scaling Approach:**
1. **Horizontal Scaling:** Deploy multiple frontend instances
2. **CDN Distribution:** Serve static files from edge locations
3. **No Coordination:** Stateless design requires no coordination
4. **Load Distribution:** Automatic via DNS/CDN

**Scaling Triggers:**
- Increased user traffic
- Geographic distribution needs
- Performance requirements

**Scaling Benefits:**
- No shared state to manage
- No session affinity required
- Easy to add/remove instances
- Cost-effective scaling

### 6.2 Backend Scaling

**Current Capacity:** Single instance  
**Target Capacity:** Multiple instances behind load balancer

**Scaling Approach:**
1. **Horizontal Scaling:** Deploy multiple backend instances
2. **Load Balancing:** Distribute requests across instances
3. **Stateless API:** No session affinity required
4. **Independent Scaling:** Scale independently from frontend

**Scaling Triggers:**
- Increased search load
- Performance degradation
- High availability requirements

**Scaling Benefits:**
- Improved performance
- High availability
- Fault tolerance
- Independent from frontend

### 6.3 Scaling Considerations

**Stateless Design:**
- No server-side sessions
- Each request is independent
- No shared state between instances
- Enables easy horizontal scaling

**Load Balancing:**
- Round-robin distribution
- Health check integration
- Automatic failover
- No session affinity needed

**Monitoring:**
- Track request rates
- Monitor response times
- Alert on errors
- Scale based on metrics

---

## 7. Data Flow

### 7.1 Search Flow

```
User Input
    │
    ▼
SearchBar Component
    │
    ▼
useSearch Hook
    │
    ├─ Validate query
    ├─ Set loading state
    ├─ Track start time
    │
    ▼
searchApi.search()
    │
    ├─ Build URL: /search?q={query}&k={limit}
    ├─ Make HTTP GET request
    ├─ Handle timeout (5s)
    │
    ▼
Backend Service
    │
    ├─ Process search query
    ├─ Return results
    │
    ▼
searchApi.search() Response
    │
    ├─ Parse JSON response
    ├─ Validate response structure
    │
    ▼
useSearch Hook
    │
    ├─ Calculate latency
    ├─ Update results state
    ├─ Update metrics state
    ├─ Clear error state
    ├─ Set loading to false
    │
    ▼
App.tsx
    │
    ├─ Pass results to ResultsList
    ├─ Pass metrics to MetricsPanel
    │
    ▼
UI Updates
```

### 7.2 Health Check Flow

```
Component Mount
    │
    ▼
useHealthCheck Hook
    │
    ├─ Initialize health = 'unknown'
    ├─ Set up polling interval (5s)
    │
    ▼
Initial Health Check
    │
    ├─ Call searchApi.checkHealth()
    ├─ Timeout: 2 seconds
    │
    ▼
Backend Service
    │
    ├─ Return 200 OK or error
    │
    ▼
useHealthCheck Hook
    │
    ├─ Update health status
    ├─ Update lastChecked timestamp
    │
    ▼
Polling Loop (every 5s)
    │
    ├─ Call searchApi.checkHealth()
    ├─ Update health status
    ├─ Continue until unmount
    │
    ▼
Component Unmount
    │
    ├─ Clear polling interval
    ├─ Prevent state updates
```

### 7.3 Error Flow

```
API Call Fails
    │
    ▼
Error Type Detection
    │
    ├─ NetworkError → "Cannot connect to server"
    ├─ TimeoutError → "Request timeout"
    ├─ HttpError → "Search failed with error {status}"
    ├─ InvalidResponseError → "Invalid server response"
    │
    ▼
useSearch Hook
    │
    ├─ Set error message
    ├─ Clear results
    ├─ Clear metrics
    ├─ Set loading to false
    │
    ▼
App.tsx
    │
    ├─ Pass error to ResultsList
    │
    ▼
ResultsList Component
    │
    ├─ Display error message
    ├─ Hide results
```

---

## 8. Error Handling Strategy

### 8.1 Error Types

**Network Errors:**
- Connection refused
- DNS resolution failure
- Network timeout
- **Handling:** Display "Cannot connect to server"

**Timeout Errors:**
- Request exceeds timeout (5s for search, 2s for health)
- **Handling:** Display "Request timeout"

**HTTP Errors:**
- 4xx: Client errors (bad request, not found)
- 5xx: Server errors (internal error, unavailable)
- **Handling:** Display "Search failed with error {status}"

**Invalid Response Errors:**
- Malformed JSON
- Missing required fields
- Invalid data types
- **Handling:** Display "Invalid server response"

### 8.2 Error Handling Principles

**Graceful Degradation:**
- Application continues to function
- User sees helpful error messages
- No crashes or white screens
- Error state is recoverable

**User-Friendly Messages:**
- Avoid technical jargon
- Provide actionable information
- Suggest next steps when possible

**Error Recovery:**
- User can retry search
- User can refresh health status
- Clear button resets error state

### 8.3 Error Handling Implementation

**In Hooks:**
- Catch all errors
- Convert to user-friendly messages
- Update error state
- Never throw exceptions to UI

**In Components:**
- Display error messages
- Provide retry mechanisms
- Show error state visually

---

## 9. Security Architecture

### 9.1 Current Security Posture

**Authentication:** None (public API)  
**Authorization:** None (public API)  
**Encryption:** HTTP (not HTTPS)  
**Input Validation:** Client-side only  

### 9.2 Security Recommendations

**HTTPS:**
- Use HTTPS in production
- Encrypt all API communication
- Prevent man-in-the-middle attacks

**Input Validation:**
- Client-side validation (UX)
- Server-side validation (security)
- Sanitize user input
- Prevent injection attacks

**CORS:**
- Configure CORS on backend
- Allow only trusted origins
- Prevent unauthorized access

**Rate Limiting:**
- Implement on backend
- Prevent abuse
- Protect against DDoS

**Future Enhancements:**
- API key authentication
- User authentication
- Role-based access control
- Audit logging

---

## 10. Monitoring & Observability

### 10.1 Frontend Monitoring

**Metrics to Track:**
- Search request count
- Search success rate
- Search latency (p50, p95, p99)
- Error rate by type
- Health check success rate
- User interactions

**Implementation:**
- Browser console logging (dev)
- Analytics integration (future)
- Error tracking service (future)

### 10.2 Backend Monitoring

**Metrics to Track:**
- Request rate
- Response time
- Error rate
- Health check status
- Resource utilization

**Implementation:**
- Backend logging
- Metrics collection
- Alerting system

### 10.3 Health Monitoring

**Health Check Strategy:**
- Poll every 5 seconds
- Display status to users
- Alert on unhealthy state
- Track uptime

**Health Indicators:**
- Green: Backend healthy
- Red: Backend unhealthy
- Gray: Unknown status

---

## Appendices

### Appendix A: Service Dependencies

```
Frontend (React App)
    │
    ├─ Depends on: Backend API
    ├─ Independent: Can run without backend (shows errors)
    └─ No dependencies on: Database, Auth service, etc.
```

### Appendix B: Deployment Checklist

- [ ] Frontend built and tested
- [ ] Backend service running
- [ ] API endpoints accessible
- [ ] Health checks working
- [ ] Error handling tested
- [ ] Monitoring configured
- [ ] Documentation updated

---

**END OF ARCHITECTURE DOCUMENT**

**Document ID:** ARCH-PHASE2-GUI-MICROSERVICES-001  
**Version:** 1.0  
**Status:** DRAFT  
**Next Document:** useSearch_SPEC.md
