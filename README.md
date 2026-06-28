# Search Platform - Solution Architecture Overview

## 1. What We're Building

A **cloud-deployable, production-grade Search Platform** with C++ core search capabilities:

- Indexes documents into an on-disk inverted index
- Serves queries with relevance ranking (BM25)
- Exposes a REST API
- Features a React GUI that evolves each phase to validate capabilities
- Includes user registration, sessions, RBAC security, observability, performance tuning, and cloud deployment

---

## 2. End-to-End Request Flow

```
┌─────────────┐
│   React UI  │  1. User types query
└──────┬──────┘
       │
       ▼
┌─────────────────────┐
│  Middle Tier        │  2. Validates identity/session
│  (API/Auth Layer)   │  3. Authorizes access
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│  C++ Search Service │  4. Reads on-disk index (mmap)
│                     │  5. Ranks results (BM25)
│                     │  6. Returns JSON
└──────┬──────────────┘
       │
       ▼
┌─────────────┐
│   React UI  │  7. Renders results + metrics
└─────────────┘
```

---

## 3. Project Phases (0-10)

### Current Status
- ✅ **Completed:** Phase 0-1
- ✅ **Completed:** Phase 2 (substeps 2.1-2.5) - persistence, CLI/runtime lifecycle, operational guarantees, PDF + OCR ingestion
- ✅ **Completed:** Phase 2 GUI - custom hooks (useSearch, useHealthCheck, useDebounce)
- ⏭️ **Next:** Phase 3 (Ranking & Query Engine)

### Phase Breakdown

| Phase | Name | Description | Key Deliverables |
|-------|------|-------------|------------------|
| **0** | Foundation | Repo structure, build system, CI, logging | CMake setup, Catch2 tests, CI pipeline |
| **1** | Core Indexing Library | In-memory indexing engine | Tokenizer, inverted index, basic queries |
| **2** | Persistent Index + Correctness | Disk persistence, serve mode, hardening | Disk I/O, lifecycle management, TDD |
| **3** | Ranking & Query Engine | BM25 relevance scoring | BM25 scoring, query semantics, reference validation |
| **4** | HTTP Service | REST API implementation | Endpoints, JSON I/O, error handling |
| **5** | Identity + Sessions + RBAC | Security layer | User auth, session mgmt, role-based access |
| **6** | Observability | Monitoring and diagnostics | Metrics, logs, tracing hooks |
| **7** | Performance + Scalability | Optimization and load handling | Profiling, caching, concurrency |
| **8** | Cloud Deployment | Containerization and orchestration | Docker, Kubernetes, cloud config |
| **9** | Advanced Product UX | Rich search features | Facets, autocomplete, filters |
| **10** | Masterclass Features *(optional)* | Advanced IR capabilities | Synonyms, LTR, advanced ranking |

---

## 4. Phase 2 Deep Dive (Complete)

Phase 2 establishes production-ready persistence and correctness:

### 2.1 - Persistent Index Format
- Define serialization format for terms/postings/documents
- Implement deterministic save/load operations
- Ensure cross-platform compatibility

### 2.2 - Serve Mode Lifecycle
- Introduce long-running server mode
- Proper startup/shutdown sequences
- Exit codes and health/readiness checks

### 2.3 - Query Correctness Baseline
- Term queries with exact matching
- AND queries (multi-term intersection)
- Deterministic result ordering

### 2.4 - Test-Driven Hardening
- Edge case coverage (empty queries, missing terms, large datasets)
- Error path validation
- Code refactoring for maintainability
- Integration test suite

### 2.5 - Deterministic PDF + OCR Ingestion
- Folder-based ingestion of mixed `.txt` and `.pdf` content
- Page-level PDF indexing (one document per page)
- Deterministic OCR policy (no user toggle; triggered by MIN_TEXT_CHARS / MIN_TOKEN_COUNT)
- Deterministic docId assignment and traversal order (UTF-8 byte-order sorted paths)
- Bounded worker/OCR thread pools, CPU-safe parallel indexing
- Additive metadata: `source_path`, `file_name`, `file_type`, `page_number`, `did_ocr`
- Search responses include `file_name` and `page_number` per hit
- Spec: `specs/phase2_5_deterministic_pdf_ocr_ingestion.md`

**Parallel Track:** React GUI development begins to validate each substep

---

## 5. Technology Stack

### Backend (Search Core)
```
Language:       C++
Build System:   CMake
Testing:        Catch2
Storage:        On-disk index files (mmap for performance)
Runtime:        Long-running server process (serve mode)

Core Modules:
├── Tokenizer
├── Inverted Index
├── Query Engine
├── Ranking (BM25)
└── Storage Layer
```

### Middle Tier (API + Auth Gateway)
```
Purpose:        Identity, sessions, RBAC, security policies
                Gateway to C++ search service

Options:
├── Node.js/Express  (fastest iteration)
├── Go               (performance + clean design)
└── C++              (possible, but more effort)

Auth Strategy:
├── External IdP     (Okta, Auth0, Azure AD, Cognito)
└── Self-managed     (Postgres users + Redis sessions)
```

### Frontend (GUI)
```
Framework:      React
Evolution:      Starts as validation harness → grows to product UI

Phase-Driven Features:
├── Phase 2:    Basic search + correctness views
├── Phase 3:    Ranking visibility + scoring debug
├── Phase 4:    API integration + latency/health metrics
├── Phase 5:    Login + RBAC UI
└── Phase 6+:   Dashboards, metrics visualization
```

### Cloud & Deployment (Phase 8+)
```
Containerization:   Docker
Orchestration:      Kubernetes
Secrets Management: Cloud secret manager (AWS Secrets Manager, etc.)
Observability:      Prometheus/Grafana pattern
                    Distributed tracing (Jaeger/Zipkin)
```

---

## 6. Key Architectural Decisions

### Why C++ for Search Core?
- Performance: sub-millisecond query latency requirements
- Control: fine-grained memory and I/O optimization
- Learning: deep understanding of search internals

### Why On-Disk Index?
- Scalability: indexes larger than RAM
- Persistence: no rebuild on restart
- Performance: mmap enables OS-level caching

### Why Middle Tier?
- Security: centralized auth/authz enforcement
- Flexibility: language-agnostic gateway
- Evolution: can swap C++ backend without UI changes

### Phased Development Strategy
- Each phase is independently testable
- GUI validates each phase's capabilities
- Production-readiness increases progressively
- Can pause/pivot at any phase boundary

---

## 7. Success Criteria by Phase

| Phase | Success Metric |
|-------|----------------|
| 0-1 | Unit tests pass, builds on CI |
| 2 | Index persists correctly, serve mode stable |
| 3 | BM25 scores match reference implementation |
| 4 | REST API handles 1000 req/sec |
| 5 | Auth/authz blocks unauthorized access |
| 6 | Metrics exported to monitoring system |
| 7 | 99th percentile latency < 50ms at scale |
| 8 | Deploys to K8s cluster with zero downtime |
| 9 | User acceptance testing passes |

---

## 8. Repository Structure (Anticipated)

```
search-platform/
├── backend/
│   ├── src/           # C++ search core
│   ├── tests/         # Catch2 unit/integration tests
│   └── CMakeLists.txt
├── middle-tier/
│   ├── src/           # API gateway + auth
│   └── tests/
├── frontend/
│   ├── src/           # React application
│   └── public/
├── infra/
│   ├── docker/        # Dockerfiles
│   └── k8s/           # Kubernetes manifests
├── docs/              # Architecture docs, ADRs
└── .github/workflows/ # CI/CD pipelines
```

---

## 9. Next Steps (Immediate)

1. **Write Phase 3 spec** - Ranking & Query Engine (formalize BM25 scoring, relevance tuning, reference-implementation validation)
2. **Update React UI** - Add ranking score visualization for Phase 3
3. **Documentation** - Create API specification for Phase 4

---

## 10. Open Questions for Discussion

- [ ] Middle tier language selection (Node.js vs Go vs C++)
- [ ] External IdP vs self-managed auth
- [ ] Cloud provider choice (AWS, GCP, Azure)
- [ ] Monitoring stack (Prometheus/Grafana vs vendor solution)
- [ ] CI/CD tooling (GitHub Actions vs Jenkins vs GitLab CI)

---

**Last Updated:** June 2026  
**Phase Status:** 2.5 Complete (PDF + OCR Ingestion)  
**Next Milestone:** Phase 3 - Ranking & Query Engine

## Phase 2 GUI - Custom Hooks ✅

Successfully implemented custom React hooks following TDD methodology:

- **useSearch** - Manages search state and API calls
- **useHealthCheck** - Monitors backend health with auto-polling
- **useDebounce** - Generic value debouncing utility

**Metrics:**
- 52 test cases (100% passing)
- 68% code reduction in App.tsx
- Microservices-ready architecture
- Full TypeScript strict mode compliance

**Documentation:** See `docs/` folder for requirements and specifications.



