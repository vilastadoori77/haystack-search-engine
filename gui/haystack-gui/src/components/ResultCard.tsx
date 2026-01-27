/**
 * ResultCard Component
 * 
 * Displays a single search result in a card format.
 * Shows the rank, document ID, relevance score, and snippet text.
 * 
 * Features:
 * - Displays rank as "#1", "#2", etc.
 * - Shows document ID as "Document {docId}"
 * - Formats score to 3 decimal places using formatScore utility
 * - Displays full snippet text (handles long snippets)
 * - Includes accessibility attributes (role="article", aria-labels)
 * 
 * @param result - The search result object containing docId, score, and snippet
 * @param rank - The rank/position of this result (1-based index)
 */
import { formatScore } from '../utils/formatters';
import { SearchResult } from '../types/search';

export interface ResultCardProps {
  result: SearchResult;
  rank: number;
}

export default function ResultCard({ result, rank }: ResultCardProps) {
  return (
    <article
      role="article"
      aria-label={`Search result ${rank}`}
      className="border border-gray-200 rounded-lg p-4 mb-4 bg-white shadow-sm"
    >
      <div className="flex items-start justify-between mb-2">
        <div className="flex items-center gap-2">
          <span className="font-bold text-gray-700">#{rank}</span>
          <span className="text-gray-600">Document {result.docId}</span>
        </div>
        <span
          aria-label={`Relevance score ${formatScore(result.score)}`}
          className="text-sm font-mono text-gray-500"
        >
          {formatScore(result.score)}
        </span>
      </div>
      <p className="text-gray-800">{result.snippet}</p>
    </article>
  );
}
