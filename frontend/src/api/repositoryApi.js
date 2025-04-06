// src/api/repositoryApi.js
const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:8080';

const handleResponse = async (response) => {
    if (!response.ok) {
        let errorMsg = `HTTP error! Status: ${response.status}`;
        let errorData = null;
        try {
            errorData = await response.json(); // Try to get JSON error details
            errorMsg = errorData?.message || errorData?.error || errorMsg;
        } catch (e) {
            try { // Fallback to text if JSON fails
                errorMsg = await response.text() || errorMsg;
            } catch (textErr) { /* ignore */ }
        }
        const error = new Error(errorMsg);
        error.status = response.status;
        error.data = errorData; // Attach full error data if available
        throw error;
    }
    // Handle different content types
    const contentType = response.headers.get("content-type");
    if (contentType && contentType.includes("application/json")) {
        return response.json();
    }
    if (contentType && contentType.includes("text/plain")) {
        return response.text();
    }
    // Default or no content
    return response.text(); // Or handle ArrayBuffer etc. if needed
};

const fetchApi = (url, options = {}) => {
    return fetch(url, options).then(handleResponse);
};

// --- Repository ---
export const getAllRepositories = () => fetchApi(`${API_BASE_URL}/api/repos`);

// --- Branches ---
export const getBranches = (repoName) => fetchApi(`${API_BASE_URL}/api/repos/${repoName}/branches`);

// --- Tags ---
export const getTags = (repoName) => fetchApi(`${API_BASE_URL}/api/repos/${repoName}/tags`);

// --- Code/Tree ---
export const getTree = (repoName, ref = 'HEAD', path = '', recursive = false) => {
    const encodedRef = encodeURIComponent(ref);
    const urlPath = path ? `/${encodeURIComponent(path)}` : '';
    const url = `${API_BASE_URL}/api/repos/${repoName}/tree${urlPath}?ref=${encodedRef}&recursive=${recursive}`;
    return fetchApi(url);
};

export const getBlobContent = (repoName, ref = 'HEAD', filePath, options = {}) => { // Accept options
    const encodedRef = encodeURIComponent(ref);
    // --- More Robust Path Encoding ---
    // Ensure filePath is split and each part encoded individually
    const pathParts = filePath.split('/').map(part => encodeURIComponent(part));
    const encodedPath = pathParts.join('/'); // Re-join encoded segments
    // --- End Robust Encoding ---

    const url = `${API_BASE_URL}/api/repos/${repoName}/blob/${encodedPath}?ref=${encodedRef}`;
    console.log("Fetching Blob URL:", url, "with options:", options); // Log options including signal
    return fetchApi(url, options); // Pass options (including signal) to fetchApi
};

// --- Commits ---
export const getCommits = (repoName, ref = 'HEAD', graph = false, limit, skip) => {
    const params = new URLSearchParams({ ref, graph });
    if (limit !== undefined) params.append('limit', limit);
    if (skip !== undefined) params.append('skip', skip);
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/commits?${params.toString()}`);
};

export const getCommitDetails = (repoName, commitSha) => {
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/commits/${encodeURIComponent(commitSha)}`);
};

// --- Objects ---
export const resolveRef = (repoName, ref) => {
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/objects/_resolve?ref=${encodeURIComponent(ref)}`);
};

export const getObjectInfo = (repoName, objectSha, operation) => {
    // Operation 't' or 's'
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/objects/${encodeURIComponent(objectSha)}?op=${operation}`);
};