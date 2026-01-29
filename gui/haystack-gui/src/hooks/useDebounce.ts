import { useState, useEffect } from 'react';


/**
 * Debounces a value, delaying updates until the value has not changed
 * for the specified delay period.
 * 
 * @template T - The type of the value to debounce
 * @param value - The value to debounce
 * @param delay - Delay in milliseconds before updating debounced value
 * @returns The debounced value (same type as input)
 * 
 * @example
 * const [query, setQuery] = useState('');
 * const debouncedQuery = useDebounce(query, 300);
 * 
 * // query changes immediately
 * // debouncedQuery changes 300ms after query stops changing
 */

export function useDebounce<T>(value: T, delay: number): T {
    // 1. Create state for debouncedValue (initilize with value )
    const [debouncedValue, setDebouncedValue] = useState<T>(value);
    //2. useEffect to:
    // set a timer to update debouncedValue after the specified delay
    //return a cleanup function to clear the timer if value or delay changes before the timer completes

    useEffect(() => {
        const timer = setTimeout(() => {
            setDebouncedValue(value);
        }, delay);

        return () => {
            clearTimeout(timer);
        };
    }, [value, delay]);
    // 3. return debouncedValue
    return debouncedValue;
}