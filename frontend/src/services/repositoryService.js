const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:8080';

/**
 * Fetches the list of repository names from the backend.
 * @returns {Promise<string[]>} A promise that resolves to an array of repository names.
 * @throws {Error} If the fetch operation fails or the response is not ok.
 */
export const getAllRepositories = async () => {
    try {
        const response = await fetch(`${API_BASE_URL}/getAllRepositories`);

        if (!response.ok) {
            let errorMsg = `HTTP error! Status: ${response.status}`;
            try {
                const errorData = await response.json();
                errorMsg = errorData.message || errorMsg;
            } catch (e) {  }
            throw new Error(errorMsg);
        }

        const data = await response.json();
        return Array.isArray(data) ? data : [];

    } catch (error) {
        console.error("Failed to fetch repositories:", error);
        throw error;
    }
};