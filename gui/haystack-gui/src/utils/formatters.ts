/**
 * Formats a score to 3 decimal places
 * 
 * @param score - The numeric score to format
 * @returns Formatted string with exactly 3 decimal places
 */

export function formatScore(score: number): string {
    // Use round half up (standard mathematical rounding)
    // Multiply by 1000, add 0.5, floor, then divide by 1000
    const rounded = Math.floor(score * 1000 + 0.5) / 1000;
    return rounded.toFixed(3);
}

/**
 * Formats a timestamp as relative time (e.g., "5s ago", "2m ago")
 * 
 * @param date - The date to format (can be null)
 * @returns Formatted relative time string
 */

export function formatTimestamp(date: Date | null): string {
    // Handle null date
    if (date === null || date === undefined) {
        return 'Never';
    }
    const now = new Date();
    const diffMs = now.getTime() - date.getTime();

    // Handle future dates or very recent times
    if (diffMs < 0) {
        return '0s ago';
    }

    const diffSeconds = Math.floor(diffMs / 1000);
    const diffMinutes = Math.floor(diffMs / (60 * 1000));
    const diffHours = Math.floor(diffMs / (60 * 60 * 1000));
    const diffDays = Math.floor(diffMs / (24 * 60 * 60 * 1000));

    // Return appropriate format based on time difference
    if (diffDays >= 1) {
        return `${diffDays}d ago`;
    }

    if (diffHours >= 1) {
        return `${diffHours}h ago`;
    }

    if (diffMinutes >= 1) {
        return `${diffMinutes}m ago`;
    }

    return `${diffSeconds}s ago`;
}
