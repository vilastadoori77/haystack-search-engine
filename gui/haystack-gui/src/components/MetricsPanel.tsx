/**
 * MetricsPanel Component
 * 
 * Displays search performance metrics after a successful search.
 * Shows the query that was executed, number of results found, and API latency.
 * 
 * Features:
 * - Conditionally renders only when metrics are available (returns null if metrics is null)
 * - Displays query text with label "Query: {query}"
 * - Shows result count with proper pluralization (1 result vs 2 results)
 * - Displays latency in milliseconds with label "Latency: {latency}ms"
 * - Includes accessibility attributes (role="region", aria-label)
 * 
 * @param metrics - Search metrics object containing query, resultCount, and latency, or null
 */
interface SearchMetrics {
  query: string;
  resultCount: number;
  latency: number; // milliseconds
}

export interface MetricsPanelProps {
  metrics: SearchMetrics | null;
}

export default function MetricsPanel({ metrics }: MetricsPanelProps) {
  // TC-040: Does not render when metrics is null
  if (metrics === null) {
    return null;
  }

  // TC-044: Handle plural/singular for result count
  const resultText = metrics.resultCount === 1 ? 'result' : 'results';

  return (
    <section
      role="region"
      aria-label="Search metrics"
      className="bg-blue-50 border border-blue-200 rounded-lg p-4 mb-4"
    >
      <div className="space-y-2">
        {/* TC-041: Displays query text */}
        <p className="text-sm text-gray-700">
          <span className="font-semibold">Query:</span> {metrics.query}
        </p>
        
        {/* TC-042: Displays result count */}
        <p className="text-sm text-gray-700">
          <span className="font-semibold">{metrics.resultCount} {resultText} found</span>
        </p>
        
        {/* TC-043: Displays latency in milliseconds */}
        <p className="text-sm text-gray-700">
          <span className="font-semibold">Latency:</span> {metrics.latency}ms
        </p>
      </div>
    </section>
  );
}
