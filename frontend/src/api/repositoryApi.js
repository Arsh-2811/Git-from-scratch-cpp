// src/api/repositoryApi.js

const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:8080';

const handleResponse = async (response) => {
    if (!response.ok) {
        let errorMsg = `HTTP error! Status: ${response.status}`;
        let errorData = null;
        try {
            errorData = await response.json();
            errorMsg = errorData?.message || errorData?.error || `Error ${response.status}`;
        } catch (e) {
             try {
                  const textError = await response.text();
                  if(textError) errorMsg = textError;
             } catch(textErr) { /* ignore */ }
        }
        const error = new Error(errorMsg);
        error.status = response.status;
        error.data = errorData;
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
    // Default or no content - try text as fallback
    return response.text().catch(() => null); // Handle cases with no body gracefully
};

const fetchApi = (url, options = {}) => {
    console.log(`[API Fetch] URL: ${url}, Options:`, options);
    return fetch(url, options).then(handleResponse);
};

// --- Repository ---
export const getAllRepositories = (options = {}) => fetchApi(`${API_BASE_URL}/api/repos`, options);

// --- Branches ---
export const getBranches = (repoName, options = {}) => fetchApi(`${API_BASE_URL}/api/repos/${repoName}/branches`, options);

// --- Tags ---
export const getTags = (repoName, options = {}) => fetchApi(`${API_BASE_URL}/api/repos/${repoName}/tags`, options);

// --- Code/Tree ---
// Note: Path should be the raw path, encoding handled here
export const getTree = (repoName, ref = 'HEAD', path = '', recursive = false, options = {}) => {
    const encodedRef = encodeURIComponent(ref);
    // Encode path segments individually, handle root path
    const encodedPath = path ? path.split('/').map(encodeURIComponent).join('/') : '';
    const urlPath = encodedPath ? `/${encodedPath}` : ''; // Prepend slash only if path exists
    const url = `${API_BASE_URL}/api/repos/${repoName}/tree${urlPath}?ref=${encodedRef}&recursive=${recursive}`;
    return fetchApi(url, options);
};

// Note: filePath should be the raw path, encoding handled here
export const getBlobContent = (repoName, ref = 'HEAD', filePath, options = {}) => {
    const encodedRef = encodeURIComponent(ref);
    const encodedFilePath = filePath.split('/').map(encodeURIComponent).join('/'); // Encode each part
    const url = `${API_BASE_URL}/api/repos/${repoName}/blob/${encodedFilePath}?ref=${encodedRef}`;
    return fetchApi(url, options);
};

// --- Commits ---
export const getCommits = (repoName, ref = 'HEAD', graph = false, limit, skip, options = {}) => {
    const params = new URLSearchParams({ ref, graph });
    if (limit !== undefined) params.append('limit', limit);
    if (skip !== undefined) params.append('skip', skip);
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/commits?${params.toString()}`, options);
};

export const getCommitDetails = (repoName, commitSha, options = {}) => {
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/commits/${encodeURIComponent(commitSha)}`, options);
};

// --- Objects ---
export const resolveRef = (repoName, ref, options = {}) => {
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/objects/_resolve?ref=${encodeURIComponent(ref)}`, options);
};

export const getObjectInfo = (repoName, objectSha, operation, options = {}) => {
    // Operation 't' or 's'
    return fetchApi(`${API_BASE_URL}/api/repos/${repoName}/objects/${encodeURIComponent(objectSha)}?op=${operation}`, options);
};