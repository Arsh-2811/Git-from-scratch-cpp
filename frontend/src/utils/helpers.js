// src/utils/helpers.js
import { format, parseISO } from 'date-fns';

export const shortenSha = (sha) => {
    return sha ? sha.substring(0, 7) : '';
};

export const formatCommitDate = (dateString) => {
    if (!dateString) return 'N/A';
    // Try to parse different potential formats from the commit info
    try {
        // Assuming format might be "timestamp zone" like "1678886400 +0530"
        const parts = dateString.split(' ');
        if (parts.length >= 1 && !isNaN(parts[0])) {
            const timestamp = parseInt(parts[0], 10) * 1000; // Convert seconds to ms
            return format(new Date(timestamp), 'MMM d, yyyy');
        }
        // Try ISO parsing as a fallback
        return format(parseISO(dateString), 'MMM d, yyyy');
    } catch (e) {
        console.warn("Could not parse date:", dateString, e);
        return dateString; // Return original if parsing fails
    }
};

export const getFileLanguage = (filePath) => {
    if (!filePath) return 'text';
    const extension = filePath.split('.').pop()?.toLowerCase();
    switch (extension) {
        case 'js':
        case 'jsx': return 'javascript';
        case 'ts':
        case 'tsx': return 'typescript';
        case 'py': return 'python';
        case 'java': return 'java';
        case 'c':
        case 'h': return 'c';
        case 'cpp':
        case 'hpp':
        case 'cxx': return 'cpp';
        case 'cs': return 'csharp';
        case 'go': return 'go';
        case 'rb': return 'ruby';
        case 'php': return 'php';
        case 'html': return 'html';
        case 'css': return 'css';
        case 'scss': return 'scss';
        case 'json': return 'json';
        case 'xml': return 'xml';
        case 'md': return 'markdown';
        case 'sh': return 'bash';
        case 'yaml':
        case 'yml': return 'yaml';
        case 'sql': return 'sql';
        case 'dockerfile': return 'dockerfile';
        default: return 'text'; // Default to plain text
    }
};

// Convert GraphData from API to React Flow nodes/edges
export const transformGraphData = (graphData) => {
    if (!graphData) return { nodes: [], edges: [] };

    const nodes = [];
    const edges = [];

    // Process commit nodes
    graphData.nodes?.forEach(node => {
        nodes.push({
            id: node.id,
            data: { label: node.label }, // Label includes sha, author, msg
            position: { x: 0, y: 0 }, // Position will be calculated by layout
            type: 'default', // Or create custom node types
            style: { // Example styling
                background: '#fff',
                border: '1px solid #bbb',
                borderRadius: '4px',
                padding: '5px 10px',
                fontSize: '10px',
                textAlign: 'left',
                whiteSpace: 'pre-wrap', // Allow newlines in label
            }
        });
    });

    // Process edges between commits
    graphData.edges?.forEach(edge => {
        edges.push({
            id: `e-${edge.source}-${edge.target}`,
            source: edge.source,
            target: edge.target,
            type: 'smoothstep', // Or 'default' etc.
            animated: false, // Optional animation
            style: { stroke: '#aaa' }
        });
    });

    // Process ref nodes (branches, tags, HEAD)
    graphData.refs?.forEach(ref => {
        nodes.push({
            id: ref.name, // Use unique name like "branch_main"
            data: { label: ref.label }, // Display name like "main"
            position: { x: 0, y: 0 },
            type: 'default', // Could be a custom node type for refs
            style: {
                background: ref.type === 'branch' ? '#e0f7fa' : (ref.type === 'tag' ? '#fff9c4' : '#e8f5e9'), // Colors
                border: `1px solid ${ref.type === 'branch' ? '#00bcd4' : (ref.type === 'tag' ? '#ffeb3b' : '#4caf50')}`,
                borderRadius: '15px', // Pill shape
                padding: '3px 8px',
                fontSize: '11px',
                fontWeight: 'bold',
            }
        });

        // Add edge from ref node to commit node
        edges.push({
            id: `e-${ref.name}-${ref.target}`,
            source: ref.name,
            target: ref.target,
            type: 'smoothstep',
            style: {
                stroke: ref.type === 'branch' ? '#00bcd4' : (ref.type === 'tag' ? '#ffc107' : '#4caf50'),
                strokeWidth: 1.5,
            }
        });
    });

    return { nodes, edges };
};