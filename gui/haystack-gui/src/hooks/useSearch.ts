import { useState, useCallback } from 'react';
import { search } from '../services/searchApi';
import { SearchResult, SearchResponse } from '../types/search';
import { validateQuery, validateLimit } from '../utils/validators';


interface SearchMetrics {
    query: string;
    resultCount: number;
    latency: number; // in milliseconds 
}

export function useSearch() {
    const [query, setQueryState] = useState<string>('');
    const [limit, setLimitState] = useState<number>(10);
    const [results, setResults] = useState<SearchResult[]>([]);
    const [isLoading, setIsLoading] = useState<boolean>(false);
    const [error, setError] = useState<string | null>(null);
    const [metrics, setMetrics] = useState<SearchMetrics | null>(null);

    //setQuery function
    const setQuery = useCallback((newQuery: string) => {
        setQueryState(newQuery);
    }, []);

    //setLimit function
    const setLimit = useCallback((newLimit: number) => {
        if (validateLimit(newLimit)) {
            setLimitState(newLimit);
        }
    }, []);

    const clearSearch = useCallback(() => {
        setQueryState('');
        setResults([]);
        setError(null);
        setMetrics(null);
    }, []);

    //Execute search function
    const executeSearch = useCallback(async () => {
        const trimmedQuery = query.trim();
        if (!validateQuery(trimmedQuery)) {
            return; // Early return, no error set
        }
        //2. Set loading sate and clear previous error
        setIsLoading(true);
        setError(null);
        setResults([]);
        //3.. Track start time for latency calculation
        const startTime = Date.now();

        try {

            // 4. Call the search API
            const response: SearchResponse = await search(trimmedQuery, limit);
            //5 Calulate how long the request took
            const latency = Date.now() - startTime;
            //Update results with the response
            setResults(response.results);
            //7. Update metrics state
            setMetrics({
                query: response.query,
                resultCount: response.results.length,
                latency: latency,
            });

        }
        catch (error) {
            // 8. Handle errors
            let errorMessage = 'An unknown error occurred';
            if (error instanceof Error) {
                errorMessage = error.message;
            }
            console.error('Search error:', error);
            setError(errorMessage);
            setResults([]);
            setMetrics(null);

        }
        finally {
            //9. Always turnoff loading state.      
            setIsLoading(false);
        }
    }, [query, limit]); // Dependencies: executeSearch depends on query and limit

    //Return the interface
    return {
        query,
        setQuery,
        limit,
        setLimit,
        results,
        isLoading,
        error,
        metrics,
        executeSearch,
        clearSearch,
    };
}
