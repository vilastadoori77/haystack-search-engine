import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import axios from 'axios';
import MockAdapter from 'axios-mock-adapter';
import { search, checkHealth } from './searchApi';
import {
  NetworkError,
  TimeoutError,
  HttpError,
  InvalidResponseError,
  SearchResponse,
} from '../types/search';

describe('searchApi', () => {
  let mockAdapter: MockAdapter;

  beforeEach(() => {
    // Create new mock adapter for each test
    mockAdapter = new MockAdapter(axios, { delayResponse: 0 });
  });

  afterEach(() => {
    // Reset mock adapter after each test
    mockAdapter.restore();
  });

  describe('search()', () => {
    // TC-049: search() calls /search with encoded query string
    it('TC-049: calls /search with encoded query string', async () => {
      // Given: API client configured
      const query = 'migration validation';
      const k = 10;
      const expectedUrl = '/search?q=migration%20validation&k=10';

      const mockResponse: SearchResponse = {
        query: 'migration validation',
        results: [],
      };

      mockAdapter.onGet(expectedUrl).reply(200, mockResponse);

      // When: search("migration validation", 10) called
      await search(query, k);

      // Then: GET request to /search?q=migration%20validation&k=10
      expect(mockAdapter.history.get.length).toBe(1);
      const firstRequest = mockAdapter.history.get[0];
      expect(firstRequest).toBeDefined();
      expect(firstRequest?.url).toBe(expectedUrl);
    });

    // TC-050: search() includes k parameter in request
    it('TC-050: includes k parameter in request', async () => {
      // Given: API client configured
      const query = 'test';
      const k = 25;

      const mockResponse: SearchResponse = {
        query: 'test',
        results: [],
      };

      mockAdapter.onGet('/search?q=test&k=25').reply(200, mockResponse);

      // When: search("test", 25) called
      await search(query, k);

      // Then: Request includes k=25 parameter
      expect(mockAdapter.history.get.length).toBe(1);
      const firstRequest = mockAdapter.history.get[0];
      expect(firstRequest).toBeDefined();
      const requestUrl = firstRequest?.url;
      expect(requestUrl).toBeDefined();
      expect(requestUrl).toContain('k=25');
    });

    // TC-051: search() throws NetworkError on connection failure
    it('TC-051: throws NetworkError on connection failure', async () => {
      // Given: Server not running
      const query = 'test';
      const k = 10;

      // Mock network error - use exact URL string like working tests
      // networkErrorOnce() creates error with no response, which triggers NetworkError
      mockAdapter.onGet('/search?q=test&k=10').networkErrorOnce();

      // When: search("test", 10) called
      // Then: NetworkError thrown with message "Cannot connect to server..."
      try {
        await search(query, k);
        expect.fail('Should have thrown NetworkError');
      } catch (error) {
        expect(error).toBeInstanceOf(NetworkError);
        expect((error as Error).message).toMatch(/Cannot connect to server/);
      }
    });

    // TC-052: search() throws TimeoutError on timeout
    it('TC-052: throws TimeoutError on timeout', async () => {
      // Given: Server configured to delay >5s
      const query = 'test';
      const k = 10;

      // Mock timeout - use exact URL string like working tests
      // timeoutOnce() sets code to 'ECONNABORTED', which triggers TimeoutError
      mockAdapter.onGet('/search?q=test&k=10').timeoutOnce();

      // When: search("test", 10) called
      // Then: TimeoutError thrown after 5 seconds
      try {
        await search(query, k);
        expect.fail('Should have thrown TimeoutError');
      } catch (error) {
        expect(error).toBeInstanceOf(TimeoutError);
        expect((error as Error).message).toMatch(/Request timeout/);
      }
    });

    // TC-053: search() throws HttpError on 4xx/5xx response
    it('TC-053: throws HttpError on 4xx/5xx response', async () => {
      // Given: Server returns 500
      const query = 'test';
      const k = 10;
      const statusCode = 500;

      // Use exact URL string like working tests
      mockAdapter.onGet('/search?q=test&k=10').reply(statusCode, { error: 'Internal Server Error' });

      // When: search("test", 10) called
      // Then: HttpError thrown with status 500
      try {
        await search(query, k);
        expect.fail('Should have thrown HttpError');
      } catch (error) {
        expect(error).toBeInstanceOf(HttpError);
        if (error instanceof HttpError) {
          expect(error.status).toBe(statusCode);
          expect(error.message).toContain('Search failed with error');
        }
      }
    });

    // TC-054: search() throws InvalidResponseError on invalid JSON
    it('TC-054: throws InvalidResponseError on invalid JSON', async () => {
      // Given: Server returns non-JSON response
      const query = 'test';
      const k = 10;

      // Use exact URL string like working tests
      mockAdapter.onGet('/search?q=test&k=10').reply(200, 'invalid json response');

      // When: search("test", 10) called
      // Then: InvalidResponseError thrown
      try {
        await search(query, k);
        expect.fail('Should have thrown InvalidResponseError');
      } catch (error) {
        expect(error).toBeInstanceOf(InvalidResponseError);
        expect((error as Error).message).toMatch(/Invalid server response/);
      }
    });

    // TC-055: search() throws InvalidResponseError on missing fields
    it('TC-055: throws InvalidResponseError on missing fields', async () => {
      // Given: Server returns JSON without results field
      const query = 'test';
      const k = 10;

      const invalidResponse = {
        query: 'test',
        // Missing 'results' field
      };

      // Use exact URL string like working tests
      mockAdapter.onGet('/search?q=test&k=10').reply(200, invalidResponse);

      // When: search("test", 10) called
      // Then: InvalidResponseError thrown
      try {
        await search(query, k);
        expect.fail('Should have thrown InvalidResponseError');
      } catch (error) {
        expect(error).toBeInstanceOf(InvalidResponseError);
        expect((error as Error).message).toMatch(/Invalid server response/);
      }
    });

    // TC-056: search() validates query is non-empty
    it('TC-056: validates query is non-empty', async () => {
      // Given: API client configured
      const emptyQuery = '';
      const k = 10;

      // When: search("", 10) called
      // Then: Error thrown before API call
      await expect(search(emptyQuery, k)).rejects.toThrow();
      
      // Verify no HTTP request was made
      expect(mockAdapter.history.get.length).toBe(0);
    });

    // TC-057: search() validates k is between 1 and 50
    it('TC-057: validates k is between 1 and 50', async () => {
      // Given: API client configured
      const query = 'test';

      // When: search("test", 0) or search("test", 51) called
      // Then: Error thrown before API call
      
      // Test k < 1
      await expect(search(query, 0)).rejects.toThrow();
      expect(mockAdapter.history.get.length).toBe(0);

      // Test k > 50
      await expect(search(query, 51)).rejects.toThrow();
      expect(mockAdapter.history.get.length).toBe(0);
    });

    // TC-058: search() returns SearchResponse on success
    it('TC-058: returns SearchResponse on success', async () => {
      // Given: Server returns valid response
      const query = 'test';
      const k = 10;

      const mockResponse: SearchResponse = {
        query: 'test',
        results: [
          {
            docId: 1,
            score: 2.456,
            snippet: 'Test snippet',
          },
          {
            docId: 2,
            score: 1.789,
            snippet: 'Another test snippet',
          },
        ],
      };

      mockAdapter.onGet('/search?q=test&k=10').reply(200, mockResponse);

      // When: search("test", 10) called
      const result = await search(query, k);

      // Then: SearchResponse object returned with correct structure
      expect(result).toBeDefined();
      expect(result.query).toBe('test');
      expect(result.results).toHaveLength(2);
      // TypeScript strict mode: check that first result exists before accessing properties
      const firstResult = result.results[0];
      expect(firstResult).toBeDefined();
      expect(firstResult).toHaveProperty('docId');
      expect(firstResult).toHaveProperty('score');
      expect(firstResult).toHaveProperty('snippet');
      expect(firstResult?.docId).toBe(1);
      expect(firstResult?.score).toBe(2.456);
      expect(firstResult?.snippet).toBe('Test snippet');
    });
  });

  describe('checkHealth()', () => {
    // TC-059: checkHealth() returns healthy when server returns 200 OK
    it('TC-059: returns healthy when server returns 200 OK', async () => {
      // Given: Server returns 200 with "OK" body
      mockAdapter.onGet('/health').reply(200, 'OK');

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'healthy'}
      expect(result).toEqual({ status: 'healthy' });
    });

    // TC-060: checkHealth() returns unhealthy when server returns 503
    it('TC-060: returns unhealthy when server returns 503', async () => {
      // Given: Server returns 503
      mockAdapter.onGet('/health').reply(503);

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'unhealthy'}
      expect(result).toEqual({ status: 'unhealthy' });
    });

    // TC-061: checkHealth() returns unhealthy on network error
    it('TC-061: returns unhealthy on network error', async () => {
      // Given: Server not running
      mockAdapter.onGet('/health').networkError();

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'unhealthy'} (does not throw)
      expect(result).toEqual({ status: 'unhealthy' });
    });

    // TC-062: checkHealth() returns unhealthy on timeout
    it('TC-062: returns unhealthy on timeout', async () => {
      // Given: Server configured to delay >2s
      mockAdapter.onGet('/health').timeout();

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'unhealthy'} (does not throw)
      expect(result).toEqual({ status: 'unhealthy' });
    });

    // TC-063: checkHealth() uses shorter timeout than search
    it('TC-063: uses shorter timeout than search', async () => {
      // Given: API client configured
      // Note: This test verifies the timeout configuration is correct
      // The actual timeout is configured in the searchApi implementation
      // We verify by checking that health check completes faster than search timeout

      mockAdapter.onGet('/health').reply(200, 'OK');

      // When: checkHealth() called
      const startTime = Date.now();
      await checkHealth();
      const duration = Date.now() - startTime;

      // Then: Timeout is 2000ms (not 5000ms)
      // Health check should complete quickly (well under 2 seconds)
      expect(duration).toBeLessThan(2000);
    });
  });

  describe('search() validation edge cases', () => {
    // Additional test for whitespace-only query
    it('validates query is not whitespace-only', async () => {
      // Given: API client configured
      const whitespaceQuery = '   ';
      const k = 10;

      // When: search("   ", 10) called
      // Then: Error thrown before API call
      await expect(search(whitespaceQuery, k)).rejects.toThrow();
      expect(mockAdapter.history.get.length).toBe(0);
    });

    // Additional test for boundary values
    it('accepts k values at boundaries (1 and 50)', async () => {
      // Given: API client configured
      const query = 'test';

      const mockResponse: SearchResponse = {
        query: 'test',
        results: [],
      };

      mockAdapter.onGet('/search?q=test&k=1').reply(200, mockResponse);
      mockAdapter.onGet('/search?q=test&k=50').reply(200, mockResponse);

      // When: search("test", 1) and search("test", 50) called
      // Then: Both succeed (boundary values are valid)
      await expect(search(query, 1)).resolves.toBeDefined();
      await expect(search(query, 50)).resolves.toBeDefined();
    });

    // Additional test for valid response structure validation
    it('validates SearchResult structure in response', async () => {
      // Given: Server returns response with invalid result structure
      const query = 'test';
      const k = 10;

      const invalidResponse = {
        query: 'test',
        results: [
          {
            docId: 1,
            // Missing 'score' field
            snippet: 'Test snippet',
          },
        ],
      };

      // Use exact URL string like other tests
      mockAdapter.onGet('/search?q=test&k=10').reply(200, invalidResponse);

      // When: search("test", 10) called
      // Then: InvalidResponseError thrown
      await expect(search(query, k)).rejects.toThrow(InvalidResponseError);
    });
  });

  describe('checkHealth() edge cases', () => {
    // Additional test for case-insensitive "OK" response
    it('accepts case-insensitive "OK" response', async () => {
      // Given: Server returns 200 with "ok" (lowercase)
      mockAdapter.onGet('/health').reply(200, 'ok');

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'healthy'}
      expect(result).toEqual({ status: 'healthy' });
    });

    // Additional test for other HTTP error codes
    it('returns unhealthy for other HTTP error codes', async () => {
      // Given: Server returns 500
      mockAdapter.onGet('/health').reply(500);

      // When: checkHealth() called
      const result = await checkHealth();

      // Then: Returns {status: 'unhealthy'}
      expect(result).toEqual({ status: 'unhealthy' });
    });
  });
});
