/**
 * API Configuration
 * 
 * Centralized configuration for API endpoints, timeouts, and base URLs.
 * Supports environment-based configuration for different deployment environments.
 */

/**
 * Base URL for the Haystack Search Engine API
 * Can be overridden via VITE_API_BASE_URL environment variable
 */
export const API_BASE_URL: string =
  import.meta.env.VITE_API_BASE_URL || 'http://localhost:8900';

/**
 * Search API timeout in milliseconds
 */
export const SEARCH_TIMEOUT: number = 5000; // 5 seconds

/**
 * Health check API timeout in milliseconds
 * Shorter than search timeout for faster failure detection
 */
export const HEALTH_TIMEOUT: number = 2000; // 2 seconds

/**
 * Health check polling interval in milliseconds
 */
export const HEALTH_POLL_INTERVAL: number = 5000; // 5 seconds

/**
 * Search endpoint path
 */
export const SEARCH_ENDPOINT: string = '/search';

/**
 * Health check endpoint path
 */
export const HEALTH_ENDPOINT: string = '/health';
