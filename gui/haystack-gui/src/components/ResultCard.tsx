/**
 * ResultCard Component
 * 
 * Displays a single search result in a card format.
 * Shows the rank, source file metadata, relevance score, and snippet text.
 * 
 * Features:
 * - Displays rank as "#1", "#2", etc.
 * - Shows Phase 2.5 source metadata (file name + page number) when present
 * - Falls back to "Document {docId}" for results without metadata
 * - Formats score to 3 decimal places using formatScore utility
 * - Displays full snippet text (handles long snippets)
 * - Includes accessibility attributes (role="article", aria-labels)
 * 
 * @param result - The search result object containing docId, score, snippet,
 *                 and optional file_name/page_number metadata
 * @param rank - The rank/position of this result (1-based index)
 */
import { formatScore } from '../utils/formatters';
import { SearchResult } from '../types/search';

export interface ResultCardProps {
  result: SearchResult;
  rank: number;
}

export default function ResultCard({ result, rank }: ResultCardProps) {
  // Pre-2.5 indexes return no metadata (file_name is absent or empty)
  const hasFileMeta = !!result.file_name;

  return (
    <article
      role="article"
      aria-label={`Search result ${rank}`}
      className="border border-gray-200 rounded-lg p-4 mb-4 bg-white shadow-sm"
    >
      <div className="flex items-start justify-between mb-2">
        <div className="flex items-center gap-2 flex-wrap">
          <span className="font-bold text-gray-700">#{rank}</span>
          {hasFileMeta ? (
            <>
              <span
                aria-label={`Source file ${result.file_name}`}
                className="text-gray-700 font-medium"
              >
                {result.file_name}
              </span>
              {result.page_number !== undefined && result.page_number > 0 && (
                <span
                  aria-label={`Page ${result.page_number}`}
                  className="text-xs text-gray-500 bg-gray-100 rounded-full px-2 py-0.5"
                >
                  Page {result.page_number}
                </span>
              )}
            </>
          ) : (
            <span className="text-gray-600">Document {result.docId}</span>
          )}
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
