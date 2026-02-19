import { useState, useEffect, useCallback, useRef } from 'react';
import { checkHealth } from '../services/searchApi';
import type { HealthCheckResponse } from '../types/search';

type HealthStatus = 'healthy' | 'unhealthy' | 'unknown';

const HEALTH_POLL_INTERVAL = 5000; // 5 seconds

export function useHealthCheck() {
    const [health, setHealth] = useState<HealthStatus>('unknown');
    const [lastChecked, setLastChecked] = useState<Date | null>(null);
    const [isRefreshing, setIsRefreshing] = useState<boolean>(false);
    const mountedRef = useRef<boolean>(true);

    // 2. Create performanceHealthCheck function
    const performHealthCheck = useCallback(async (isManual: boolean = false) => {
        if (isManual) {
            setIsRefreshing(true);
        }
        try {
            const response: HealthCheckResponse = await checkHealth();
            if (mountedRef.current) {
                setHealth(response.status);
                setLastChecked(new Date());
            }
        } catch {
            if (mountedRef.current) {
                setHealth('unhealthy');
                setLastChecked(new Date());
            }
        } finally {
            if (isManual && mountedRef.current) {
                setIsRefreshing(false);
            }
        }


    }, []);
    // 3. useEffect to perform initial health check on mount
    useEffect(() => {
        mountedRef.current = true;

        performHealthCheck(false);

        //Set up polling interval
        const intervalId = setInterval(() => {
            if (mountedRef.current) {
                performHealthCheck(false);
            }
        }, HEALTH_POLL_INTERVAL);
        //Cleanup on unmount
        return () => {
            mountedRef.current = false;
            clearInterval(intervalId);
        };
    }, [performHealthCheck]);

    //4. Create refreshHealth function
    const refreshHealth = useCallback(async () => {
        await performHealthCheck(true);
    }, [performHealthCheck]);

    // Return the interface
    return { health, lastChecked, isRefreshing, refreshHealth };
}