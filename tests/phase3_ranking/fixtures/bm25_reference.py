#!/usr/bin/env python3
"""Generate tests/phase3_ranking/fixtures/bm25_reference.json — BM25 reference (spec §2)."""

from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List, Tuple

K1 = 1.2
B = 0.75
TOL = 1e-9


def tokenize(text: str) -> List[str]:
    out: List[str] = []
    cur: List[str] = []
    for ch in text:
        if ch.isalnum():
            cur.append(ch.lower())
        else:
            if cur:
                out.append("".join(cur))
                cur = []
    if cur:
        out.append("".join(cur))
    return out


def build_index(docs: Dict[int, str]) -> Tuple[int, float, Dict[str, Dict[int, int]], Dict[int, int]]:
    tf: Dict[str, Dict[int, int]] = {}
    dl: Dict[int, int] = {}
    for doc_id, text in docs.items():
        tokens = tokenize(text)
        dl[doc_id] = len(tokens)
        seen = set()
        for t in tokens:
            tf.setdefault(t, {})
            tf[t][doc_id] = tf[t].get(doc_id, 0) + 1
            seen.add(t)
    n = len(docs)
    avgdl = sum(dl.values()) / n if n else 0.0
    return n, avgdl, tf, dl


def idf(n: int, df: int) -> float:
    return math.log(((n - df + 0.5) / (df + 0.5)) + 1.0)


def bm25_score(
    query_terms: List[str],
    doc_id: int,
    n: int,
    avgdl: float,
    tf: Dict[str, Dict[int, int]],
    dl: Dict[int, int],
) -> float:
    if avgdl <= 0.0:
        denom_norm = 1.0
    else:
        denom_norm = 1.0 - B + B * (dl[doc_id] / avgdl)
    score = 0.0
    for term in query_terms:
        df = len(tf.get(term, {}))
        if df == 0:
            continue
        tfd = tf.get(term, {}).get(doc_id, 0)
        if tfd == 0:
            continue
        idf_v = idf(n, df)
        tf_part = (tfd * (K1 + 1.0)) / (tfd + K1 * denom_norm)
        score += idf_v * tf_part
    return score


def score_query(docs: Dict[int, str], query: str) -> List[Tuple[int, float]]:
    pq_terms = []
    for part in query.split():
        if part in ("OR", "or"):
            continue
        if part.startswith("-"):
            continue
        pq_terms.extend(tokenize(part))
    n, avgdl, tf, dl = build_index(docs)
    is_or = " OR " in f" {query} " or " or " in f" {query} "
    not_terms = []
    pos_terms = []
    for part in query.split():
        if part in ("OR", "or"):
            continue
        if part.startswith("-"):
            not_terms.extend(tokenize(part[1:]))
        else:
            pos_terms.extend(tokenize(part))
    if not pos_terms:
        return []
    candidates = set()
    first = True
    for term in pos_terms:
        posting = set(tf.get(term, {}).keys())
        if first:
            candidates = posting
            first = False
        else:
            candidates = candidates | posting if is_or else candidates & posting
    excluded = set()
    for term in not_terms:
        excluded |= set(tf.get(term, {}).keys())
    scored: List[Tuple[int, float]] = []
    for doc_id in candidates:
        if doc_id in excluded:
            continue
        scored.append((doc_id, bm25_score(pos_terms, doc_id, n, avgdl, tf, dl)))
    scored.sort(key=lambda x: (-x[1], x[0]))
    return scored


def case(name: str, docs: Dict[int, str], query: str) -> dict:
    expected = [{"docId": d, "score": s} for d, s in score_query(docs, query)]
    return {
        "name": name,
        "documents": [{"docId": k, "text": v} for k, v in sorted(docs.items())],
        "query": query,
        "expected": expected,
    }


def main() -> None:
    cases = [
        case(
            "single_term",
            {1: "alpha bravo", 2: "alpha alpha bravo", 3: "charlie"},
            "alpha",
        ),
        case(
            "multi_and",
            {1: "alpha bravo", 2: "alpha charlie", 3: "bravo charlie", 4: "alpha bravo delta"},
            "alpha bravo",
        ),
        case(
            "length_norm",
            {
                1: "hello filler filler filler filler filler filler filler filler filler filler world",
                2: "hello world",
            },
            "hello world",
        ),
        case(
            "idf_rare_term",
            {
                1: "common common common rareword",
                2: "common common common common",
                3: "common common common",
            },
            "rareword common",
        ),
    ]

    # Tie-break: identical BM25 scores → ascending docId (spec §4.1).
    tie_docs = {1: "tie query token", 2: "tie query token"}
    tie_scored = score_query(tie_docs, "tie query")
    assert len(tie_scored) == 2
    assert abs(tie_scored[0][1] - tie_scored[1][1]) < TOL

    # Tie-break corpus: identical BM25 scores → ascending docId (spec §4.1).
    tie_break = {
        "name": "tie_break",
        "documents": [{"docId": k, "text": v} for k, v in sorted(tie_docs.items())],
        "query": "tie query",
        "expected": [{"docId": d, "score": s} for d, s in tie_scored],
        "require_epsilon_tie": True,
    }

    out = {
        "k1": K1,
        "b": B,
        "tolerance": TOL,
        "cases": cases,
        "tie_break": tie_break,
    }
    path = Path(__file__).with_name("bm25_reference.json")
    path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {path}")


if __name__ == "__main__":
    main()
