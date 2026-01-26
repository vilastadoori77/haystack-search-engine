/**
 * HealthStatus Component
 * 
 * Displays the health status of the Haystack Search Engine API server.
 * Shows a colored indicator dot, status text, last checked timestamp, and a refresh button.
 * 
 * Features:
 * - Visual indicator dot:
 *   - Green dot when status is "healthy"
 *   - Red dot when status is "unhealthy"
 *   - Gray dot when status is "unknown"
 * - Status text:
 *   - "Healthy" for healthy status
 *   - "Unavailable" for unhealthy status
 *   - "Unknown" for unknown status
 * - Displays relative timestamp of last health check (e.g., "2s ago", "1m ago")
 * - Shows "Never" when lastChecked is null
 * - Refresh button to manually trigger a health check
 * - Refresh button is disabled when isRefreshing is true
 * - Includes accessibility attributes (role="status", aria-labels)
 * 
 * @param health - Current health status: 'healthy', 'unhealthy', or 'unknown'
 * @param lastChecked - Date object representing when the health was last checked, or null
 * @param onRefresh - Callback function called when refresh button is clicked
 * @param isRefreshing - Boolean indicating if a health check is currently in progress
 */
import { formatTimestamp } from '../utils/formatters';

type HealthStatusType = 'healthy' | 'unhealthy' | 'unknown';

interface HealthStatusProps {
  health: HealthStatusType;
  lastChecked: Date | null;
  onRefresh: () => void;
  isRefreshing: boolean;
}

export default function HealthStatus({
  health,
  lastChecked,
  onRefresh,
  isRefreshing,
}: HealthStatusProps) {
  // Determine dot color based on health status
  const getDotColor = () => {
    switch (health) {
      case 'healthy':
        return 'bg-green-500';
      case 'unhealthy':
        return 'bg-red-500';
      default:
        return 'bg-gray-500';
    }
  };

  // Determine status text
  const getStatusText = () => {
    switch (health) {
      case 'healthy':
        return 'Healthy';
      case 'unhealthy':
        return 'Unavailable';
      default:
        return 'Unknown';
    }
  };

  return (
    <div className="flex items-center gap-3 p-3 bg-white border border-gray-200 rounded-lg">
      {/* Health indicator dot */}
      <div
        role="status"
        aria-label={`Health status: ${health}`}
        className={`w-3 h-3 rounded-full ${getDotColor()}`}
        data-testid="health-indicator"
      />
      
      {/* Status text */}
      <span className="text-sm font-medium text-gray-700">{getStatusText()}</span>
      
      {/* Last checked timestamp */}
      <span className="text-xs text-gray-500">
        Last checked: {formatTimestamp(lastChecked)}
      </span>
      
      {/* Refresh button */}
      <button
        onClick={onRefresh}
        disabled={isRefreshing}
        aria-label="Refresh health status"
        className="ml-auto px-2 py-1 text-xs bg-gray-100 hover:bg-gray-200 disabled:bg-gray-50 disabled:text-gray-400 disabled:cursor-not-allowed rounded border border-gray-300 transition-colors"
      >
        {isRefreshing ? 'Refreshing...' : 'Refresh'}
      </button>
    </div>
  );
}
