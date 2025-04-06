// src/hooks/useFetchData.js
import { useState, useEffect, useCallback } from 'react';

function useFetchData(fetchFunction, initialArgs = [], autoFetch = true) {
    const [data, setData] = useState(null);
    const [loading, setLoading] = useState(autoFetch);
    const [error, setError] = useState(null);

    // Use useCallback to memoize the fetchData function
    const fetchData = useCallback(async (...args) => {
        setLoading(true);
        setError(null);
        setData(null); // Clear previous data on new fetch
        try {
            const result = await fetchFunction(...args);
            setData(result);
        } catch (err) {
            console.error("Fetch error:", err);
            setError(err);
        } finally {
            setLoading(false);
        }
    }, [fetchFunction]); // Dependency: fetchFunction itself

    useEffect(() => {
        if (autoFetch) {
            fetchData(...initialArgs);
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [autoFetch, fetchData]); // Run on mount if autoFetch is true, args passed via fetchData call

    return { data, loading, error, fetchData };
}

export default useFetchData;