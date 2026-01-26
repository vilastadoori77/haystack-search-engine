/**
 * Validates a search query string
 * @param query - The query string to validate
 * @returns true if query is non-empty and not whitespace-only, false otherwise
 */
export function validateQuery(query: string): boolean {
  // Handle null/undefined
  if (query == null) {
    return false;
  }

  // Trim whitespace and check if empty
  return query.trim().length > 0;
}

/**
 * Validates a limit value for search results
 * @param limit - The limit value to validate
 * @returns true if limit is between 1 and 50 (inclusive) and is an integer, false otherwise
 */
export function validateLimit(limit: number): boolean {
  // Check if it's a valid number
  if (typeof limit !== 'number' || isNaN(limit)) {
    return false;
  }

  // Check if it's an integer (not a decimal)
  if (!Number.isInteger(limit)) {
    return false;
  }

  // Check if it's within the valid range (1-50)
  return limit >= 1 && limit <= 50;
}
