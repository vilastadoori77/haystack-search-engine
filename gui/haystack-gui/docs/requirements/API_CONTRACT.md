# API Contract - Haystack Search Engine

**Project:** Haystack Search Engine  
**Document Type:** API Contract Specification  
**Version:** 1.0  
**Date:** January 28, 2026  
**Status:** Draft  
**Author:** API Specification Team  

---

## Document Control

| Version | Date | Author | Status | Changes |
|---------|------|--------|--------|---------|
| 1.0 | 2026-01-28 | API Team | Draft | Initial API contract |

---

## Table of Contents

1. [Overview](#1-overview)
2. [Base URL](#2-base-url)
3. [Search API](#3-search-api)
4. [Health API](#4-health-api)
5. [Error Responses](#5-error-responses)
6. [Rate Limiting](#6-rate-limiting)
7. [Versioning](#7-versioning)

---

## 1. Overview

### 1.1 API Information

**Protocol:** HTTP/1.1  
**Format:** JSON (for search), Plain Text (for health)  
**Authentication:** None (current)  
**Base URL:** `http://localhost:8900` (development)  
**Timeout:** 
- Search: 5000ms
- Health: 2000ms

### 1.2 API Endpoints

| Endpoint | Method | Purpose | Timeout |
|----------|--------|---------|---------|
| `/search` | GET | Perform search query | 5000ms |
| `/health` | GET | Check service health | 2000ms |

---

## 2. Base URL

### 2.1 Environment Configuration

**Development:**
```
http://localhost:8900
```

**Production:**
```
https://api.haystack.example.com
```
(Configurable via environment variable)

### 2.2 URL Structure

```
{baseUrl}/{endpoint}?{queryParameters}
```

---

## 3. Search API

### 3.1 Endpoint

**URL:** `GET /search`  
**Purpose:** Execute search query and return matching documents

### 3.2 Request Parameters

**Query Parameters:**

| Parameter | Type | Required | Description | Constraints |
|-----------|------|----------|-------------|-------------|
| `q` | string | Yes | Search query | Non-empty, URL encoded |
| `k` | number | Yes | Number of results | Integer, 1-50 |

**Example Request:**
```
GET /search?q=migration&k=10
```

**URL Encoding:**
- Spaces: `%20` or `+`
- Special characters: URL encoded
- Example: `q=migration%20validation`

### 3.3 Success Response

**Status Code:** `200 OK`  
**Content-Type:** `application/json`

**Response Body:**
```json
{
  "query": "migration",
  "results": [
    {
      "docId": 1,
      "score": 0.95,
      "snippet": "Document snippet text..."
    },
    {
      "docId": 2,
      "score": 0.87,
      "snippet": "Another document snippet..."
    }
  ]
}
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `query` | string | The search query (may be normalized) |
| `results` | array | Array of search results |
| `results[].docId` | number | Document identifier |
| `results[].score` | number | Relevance score (0.0-1.0) |
| `results[].snippet` | string | Text snippet from document |

### 3.4 Error Responses

**400 Bad Request:**
- Invalid query parameter
- Invalid k parameter
- Missing required parameters

**500 Internal Server Error:**
- Server processing error
- Database error

**503 Service Unavailable:**
- Service temporarily unavailable
- Maintenance mode

---

## 4. Health API

### 4.1 Endpoint

**URL:** `GET /health`  
**Purpose:** Check if the backend service is healthy and available

### 4.2 Request Parameters

None

**Example Request:**
```
GET /health
```

### 4.3 Success Response

**Status Code:** `200 OK`  
**Content-Type:** `text/plain`

**Response Body:**
```
OK
```

**Note:** Response body is case-insensitive ("OK", "ok", "Ok" all valid)

### 4.4 Error Responses

**503 Service Unavailable:**
- Service is unhealthy
- Service is down
- Service is in maintenance mode

**Other Errors:**
- Network errors
- Timeout errors
- Any non-200 response

**All errors interpreted as 'unhealthy' by frontend**

---

## 5. Error Responses

### 5.1 Error Response Format

**HTTP Error (4xx/5xx):**
- Status code indicates error type
- Response body may contain error details
- Frontend interprets status code

**Network Errors:**
- Connection refused
- DNS resolution failure
- No response received

**Timeout Errors:**
- Request exceeds timeout
- Search: >5000ms
- Health: >2000ms

### 5.2 Error Handling

**Frontend Behavior:**
- Catch all errors
- Convert to user-friendly messages
- Display error in UI
- Never crash application
- Allow retry

---

## 6. Rate Limiting

### 6.1 Current Limits

**No rate limiting implemented** (current)

**Recommendations:**
- Implement rate limiting on backend
- Return 429 Too Many Requests when exceeded
- Frontend should handle gracefully

---

## 7. Versioning

### 7.1 Current Version

**Version:** v1 (implicit)  
**No versioning in URL** (current)

**Future Considerations:**
- Add version to URL: `/v1/search`
- Support multiple API versions
- Deprecation strategy

---

## Appendices

### Appendix A: Request Examples

**Search Request:**
```bash
curl "http://localhost:8900/search?q=migration&k=10"
```

**Health Request:**
```bash
curl "http://localhost:8900/health"
```

### Appendix B: Response Examples

**Successful Search:**
```json
{
  "query": "migration",
  "results": [
    {
      "docId": 1,
      "score": 0.95,
      "snippet": "This document discusses database migration strategies..."
    }
  ]
}
```

**Empty Results:**
```json
{
  "query": "nonexistent",
  "results": []
}
```

---

**END OF API CONTRACT**

**Document ID:** API-CONTRACT-001  
**Version:** 1.0  
**Status:** DRAFT
