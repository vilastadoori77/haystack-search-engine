# Haystack — Commercial Roadmap & Product Strategy

**Last updated:** 2026-06-28
**Status:** Phase 2.5 complete and verified end-to-end. Going commercial.

---

## 1. What the product is

A **self-hostable, document-focused search platform** built on a C++ core:

- Inverted index + BM25 ranking (lexical search)
- On-disk persistence, serve mode, REST API, React GUI
- **Built-in PDF + OCR ingestion with page-level indexing** ← the differentiator

It is in the same *family* as Elasticsearch, Solr, Typesense, and Meilisearch, but
with native OCR/document ingestion that those tools do not provide out of the box.

---

## 2. Where it stands vs existing products

| Capability | Haystack | Meilisearch / Typesense | Elasticsearch / Solr | Algolia (SaaS) |
|---|---|---|---|---|
| Inverted index | ✅ | ✅ | ✅ | ✅ |
| BM25 ranking | ⬜ Phase 3 (implemented; formalization in progress) | ✅ | ✅ | ✅ |
| Disk persistence | ✅ | ✅ | ✅ | ✅ |
| REST API | ✅ basic | ✅ | ✅ rich | ✅ |
| **PDF + OCR ingestion** | ✅ **built-in** | ❌ | ❌ | ❌ |
| Auth / RBAC | ⬜ Phase 5 | ✅ | ✅ | ✅ |
| Typo tolerance / fuzzy | ❌ | ✅ | ✅ | ✅ |
| Facets / filters | ⬜ Phase 9 | ✅ | ✅ | ✅ |
| Distributed / sharding | ❌ | partial | ✅ | ✅ |
| Vector / semantic search | ⬜ Phase 11 | ✅ | ✅ | ✅ |
| Horizontal scale / HA | ⬜ Phase 7–8 | ✅ | ✅ | ✅ (managed) |

**Maturity:** comparable to an early single-node lexical engine — a solid core, not
yet a platform. Not built to compete head-on with distributed engines; the realistic
sweet spot is **document search for PDF/scan-heavy verticals**.

---

## 3. Commercial direction (decided 2026-06-28)

| Decision | Choice |
|---|---|
| **Delivery model** | Both / hybrid — on-prem (self-hosted) AND cloud SaaS |
| **Target market** | Document-heavy verticals: legal, engineering/PLM, healthcare, insurance |
| **AI / semantic** | Add hybrid (BM25 + vector) + RAG, **sequenced after** a lexical+OCR v1 |

**Wedge:** "Drop-in search for scanned/PDF-heavy document archives" — buyers with
millions of scanned pages, who value on-prem (data can't leave their network) and
are underserved by SaaS-only players. Sample data (Teamcenter/PLM) points to
**engineering/manufacturing document management** as the natural beachhead.

**Killer feature (the upsell):** ask a question in plain English → get an answer with
a **citation to the exact page of the exact document** (RAG over page-level OCR chunks).

---

## 4. What "commercial" requires beyond the technical phases

| Requirement | Notes |
|---|---|
| Multi-tenancy | Isolate many customers' data — design tenant-aware at Phase 5 |
| Billing / metering / usage limits | Required for SaaS |
| SLAs, backups, disaster recovery | Operational, partly Phase 7–8 |
| Security compliance | Encryption at rest/in transit, SOC 2 path, GDPR, audit logs |
| Admin console / onboarding / API keys | Self-service product surface |
| Support / monitoring / incident response | Operational, not code |

Commercial ≠ feature-complete. Multi-tenancy and compliance are the most commonly
underestimated items.

---

## 5. Target architecture

```
┌──────────────────────────────────────────────┐
│  React UI  +  Admin console (tenants, keys)   │
├──────────────────────────────────────────────┤
│  API / Auth Gateway — multi-tenant, RBAC,     │
│  rate limits, billing hooks      (Phase 4/5)  │
├───────────────┬───────────────┬───────────────┤
│ Lexical (BM25)│ Vector / ANN  │  RAG / LLM     │
│  C++ core     │ embeddings DB │  answer + cite │
│  (Phase 3)    │ (Phase 11a)   │  (Phase 11b)   │
├───────────────┴───────────────┴───────────────┤
│  Ingestion: PDF + OCR + chunking (Phase 2.5 ✅)│
│  → produces page-level chunks for BOTH paths   │
├──────────────────────────────────────────────┤
│  Storage: per-tenant index + object store     │
└──────────────────────────────────────────────┘
```

One ingestion pipeline feeds **two retrieval paths** (keyword + vector), combined at
query time (hybrid), topped with an LLM for cited answers.

### Architectural decisions to make now (avoid rework)

1. **Tenant-aware data model from Phase 5** — carry `tenant_id` everywhere, even for
   single-tenant on-prem, so the same codebase serves SaaS later.
2. **Do NOT hand-roll the vector/ANN index** — use FAISS or a vector DB
   (Qdrant / Weaviate). Keep the C++ core for what it is good at (lexical).
3. **Package once, deploy twice** — containerize so one artifact runs on-prem
   (customer K8s/Docker) and in your SaaS. Makes hybrid delivery affordable.

---

## 6. Revised commercial roadmap

| Stage | What | Why it's here |
|---|---|---|
| **3–4** | Ranking + clean REST API | Finish the core engine |
| **5** | **Multi-tenant** auth/RBAC + API keys | The commercial fork — design tenant-aware |
| **8 (pull early)** | Containerized deploy (on-prem + SaaS) | Enables hybrid delivery + first pilots |
| **v1 SELLABLE** | Lexical + OCR document search, self-hosted pilots | Land a paying design-partner in one vertical |
| **11a** | Vector + hybrid retrieval | Match 2026 buyer expectations |
| **11b** | RAG answers with page-level citations | The killer feature / upsell |
| **6–7** | Observability + performance / scale | Harden for production load |
| **Compliance** | Encryption, SOC 2 path, audit logs | Runs in parallel — required for regulated buyers |

---

## 7. Timeline

### Measured pace (Dec 2025 – Mar 2026)
- ~12 calendar weeks delivered Phases 0, 1, 2.0–2.5, and the GUI (~8 sub-phase units)
- Rate ≈ **1 sub-phase every ~1.5 weeks**, part-time (evenings/weekends)

### Projection at that pace

| Target | From now (2026-06-28) | Calendar finish |
|---|---|---|
| Phase 4 (ranked search + real API) | ~4 weeks | late July 2026 |
| Phase 5 (multi-tenant security) | ~9 weeks | early Sept 2026 |
| Phases 3–9 complete (full product, no optional) | ~22 weeks (~5–5.5 mo) | ~early Dec 2026 |
| Everything incl. Phase 10 | ~26 weeks (~6.5 mo) | ~late Jan 2027 |
| **Commercial v1 (multi-tenant + deploy + first AI)** | — | **~12–18 months part-time** |

> Assumes continuous pace with no idle gaps. The prior history had a ~3-month pause
> (Mar–Jun 2026); recurring gaps extend the calendar. The biggest scope jump — and
> the most likely place to slip — is **Phase 5 (multi-tenant auth/RBAC)**.

---

## 8. Most important next move

**Pick ONE vertical and land ONE design-partner customer before building further.**
A real customer's requirements will sequence the roadmap far better than any plan.
"Scanned engineering drawings + specs search with OCR" (PLM/manufacturing) is a
sharp, winnable wedge given the existing direction.
