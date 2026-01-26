// src/types/search.ts
// TypeScript interfaces for Haystack Search Engine API

/**
 * Custom error classes for API error handling
 */

export class NetworkError extends Error {
    constructor(message: string = 'Cannot connect to the Haystack Search Engine API') {
        super(message);
        this.name = 'NetworkError';
        Object.setPrototypeOf(this, NetworkError.prototype);

    }
}

export class TimeoutError extends Error {
    constructor(message: string = 'The request to the Haystack Search Engine API timed out') {
        super(message);
        this.name = 'TimeoutError';
        Object.setPrototypeOf(this, TimeoutError.prototype);
    }
}

export class HttpError extends Error {
    public readonly status: number;

    constructor(status: number, message: string = `Search failed with error ${status}`) {
        super(message);
        this.name = 'HttpError';
        this.status = status;
        Object.setPrototypeOf(this, HttpError.prototype);
    }
}

export class InvalidResponseError extends Error {
    constructor(message: string = 'Invalid server response') {
        super(message);
        this.name = 'InvalidResponseError';
        Object.setPrototypeOf(this, InvalidResponseError.prototype);
    }
}

/**
 * Search result item returned from the API
 */
export interface SearchResult {
    docId: number;
    score: number;
    snippet: string;
}

/**
 * Response from the search endpoint
 */
export interface SearchResponse {
    query: string;
    results: SearchResult[];
}

/**
 * Response from the health check endpoint
 */
export interface HealthCheckResponse {
    status: 'healthy' | 'unhealthy';
}
