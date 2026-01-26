// src/services/searchApi.ts
// API client for Haystack Search Engine

import axios, { AxiosError } from 'axios';
import {
    NetworkError,
    TimeoutError,
    HttpError,
    InvalidResponseError,
    SearchResponse,
    SearchResult,
    HealthCheckResponse,
} from '../types/search';

// Configuration constants
const SEARCH_TIMEOUT = 5000;  // 5 seconds for search
const HEALTH_TIMEOUT = 2000;  // 2 seconds for health check

/**
 * Validates query parameter
 * @throws Error if query is empty or whitespace-only
 */
function validateQuery(query: string): void {
    if (!query || query.trim().length === 0) {
        throw new Error('Query cannot be empty');
    }
}

/**
 * Validates k parameter
 * @throws Error if k is not between 1 and 50
 */
function validateK(k: number): void {
    if (k < 1 || k > 50) {
        throw new Error('k must be between 1 and 50');
    }
}

/**
 * Validates a SearchResult object has required fields
 */
function isValidSearchResult(result: unknown): result is SearchResult {
    if (typeof result !== 'object' || result === null) {
        return false;
    }
    const obj = result as Record<string, unknown>;
    return (
        typeof obj.docId === 'number' &&
        typeof obj.score === 'number' &&
        typeof obj.snippet === 'string'
    );
}

/**
 * Validates the search response structure
 * @throws InvalidResponseError if response structure is invalid
 */
function validateSearchResponse(data: unknown): SearchResponse {
    // Check if data is an object
    if (typeof data !== 'object' || data === null) {
        throw new InvalidResponseError('Invalid server response: expected object');
    }

    const response = data as Record<string, unknown>;

    // Check for required 'query' field
    if (typeof response.query !== 'string') {
        throw new InvalidResponseError('Invalid server response: missing query field');
    }

    // Check for required 'results' field
    if (!Array.isArray(response.results)) {
        throw new InvalidResponseError('Invalid server response: missing results field');
    }

    // Validate each result in the array
    for (const result of response.results) {
        if (!isValidSearchResult(result)) {
            throw new InvalidResponseError('Invalid server response: invalid result structure');
        }
    }

    return {
        query: response.query,
        results: response.results as SearchResult[],
    };
}

/**
 * Perform a search query against the Haystack API
 * 
 * @param query - The search query string (non-empty)
 * @param k - Number of results to return (1-50)
 * @returns Promise<SearchResponse> - The search results
 * @throws NetworkError - When unable to connect to server
 * @throws TimeoutError - When request exceeds 5 seconds
 * @throws HttpError - When server returns 4xx/5xx status
 * @throws InvalidResponseError - When response format is invalid
 */
export async function search(query: string, k: number): Promise<SearchResponse> {
    // Validate inputs before making request
    validateQuery(query);
    validateK(k);

    // Build URL with encoded query string
    const url = `/search?q=${encodeURIComponent(query)}&k=${k}`;

    try {
        const response = await axios.get(url, {
            timeout: SEARCH_TIMEOUT,
        });

        // Validate and return the response
        return validateSearchResponse(response.data);
    } catch (error) {
        // Re-throw our custom errors if already wrapped
        if (error instanceof InvalidResponseError ||
            error instanceof NetworkError ||
            error instanceof TimeoutError ||
            error instanceof HttpError) {
            throw error;
        }

        // Handle axios errors
        if (axios.isAxiosError(error)) {
            const axiosError = error as AxiosError;

            // Check for timeout FIRST (before checking response)
            if (axiosError.code === 'ECONNABORTED' || axiosError.code === 'ETIMEDOUT') {
                throw new TimeoutError('Request timeout after 5 seconds');
            }

            // Network error - when there's no response at all
            // axios-mock-adapter sets response to undefined for networkError()
            if (axiosError.response === undefined) {
                throw new NetworkError('Cannot connect to server');
            }

            // HTTP error (4xx/5xx response) - response exists
            if (axiosError.response) {
                throw new HttpError(
                    axiosError.response.status,
                    `Search failed with error ${axiosError.response.status}`
                );
            }
        }

        // Unknown error - wrap as NetworkError
        throw new NetworkError('Cannot connect to server');
    }
}

/**
 * Check the health status of the Haystack API
 * 
 * @returns Promise<HealthCheckResponse> - The health status
 * Note: This function never throws - it returns unhealthy on any error
 */
export async function checkHealth(): Promise<HealthCheckResponse> {
    try {
        const response = await axios.get('/health', {
            timeout: HEALTH_TIMEOUT,
        });

        // Check for 200 status and "OK" response (case-insensitive)
        if (response.status === 200) {
            const body = typeof response.data === 'string'
                ? response.data.toLowerCase()
                : '';

            if (body === 'ok') {
                return { status: 'healthy' };
            }
        }

        return { status: 'unhealthy' };
    } catch {
        // Any error (network, timeout, HTTP error) returns unhealthy
        return { status: 'unhealthy' };
    }
}