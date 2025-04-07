// src/utils/helpers.js
import { format, parseISO, isValid } from 'date-fns';
import { formatDistanceToNowStrict } from 'date-fns'; // Use stricter version

export const shortenSha = (sha) => {
    return sha ? sha.substring(0, 7) : '';
};

export const formatCommitDate = (dateString, relative = false) => {
    if (!dateString) return 'N/A';
    let date = null;
    try {
        // Assuming format might be "timestamp zone" like "1678886400 +0530"
        const parts = dateString.split(' ');
        if (parts.length >= 1 && /^\d+$/.test(parts[0])) { // Check if first part is only digits
             const timestamp = parseInt(parts[0], 10) * 1000; // Convert seconds to ms
             date = new Date(timestamp);
        } else {
             // Try ISO parsing as a fallback for other potential formats
             date = parseISO(dateString);
        }

        if (isValid(date)) { // Check if parsing resulted in a valid date
             return relative
                 ? formatDistanceToNowStrict(date, { addSuffix: true })
                 : format(date, 'MMM d, yyyy');
        } else {
             console.warn("Could not parse date:", dateString);
             return dateString; // Return original if parsing fails
        }
    } catch (e) {
        console.warn("Error parsing date:", dateString, e);
        return dateString;
    }
};


export const getFileLanguage = (filePath) => {
    if (!filePath) return 'text';
    const filename = filePath.split('/').pop()?.toLowerCase();
    const extension = filename?.split('.').pop();

    // Handle specific filenames first
    if (filename === 'dockerfile') return 'dockerfile';
    if (filename === '.gitignore') return 'gitignore';
    if (filename === 'cmakelists.txt') return 'cmake';
    if (filename?.startsWith('readme')) return 'markdown';
    if (filename === 'pom.xml') return 'xml'; // or 'maven' if supported

    switch (extension) {
        case 'js': case 'jsx': return 'javascript';
        case 'ts': case 'tsx': return 'typescript';
        case 'py': return 'python';
        case 'java': return 'java';
        case 'c': return 'c';
        case 'h': return 'c'; // Treat C headers as C
        case 'cpp': case 'hpp': case 'cxx': case 'hxx': return 'cpp';
        case 'cs': return 'csharp';
        case 'go': return 'go';
        case 'rb': return 'ruby';
        case 'php': return 'php';
        case 'html': return 'html';
        case 'css': return 'css';
        case 'scss': case 'sass': return 'scss';
        case 'json': return 'json';
        case 'xml': return 'xml';
        case 'md': return 'markdown';
        case 'sh': return 'bash';
        case 'yaml': case 'yml': return 'yaml';
        case 'sql': return 'sql';
        case 'dockerfile': return 'dockerfile';
        case 'gradle': return 'groovy'; // Gradle files often use Groovy
        case 'kt': return 'kotlin';
        case 'swift': return 'swift';
        default: return 'text';
    }
};

// Convert GraphData from API to React Flow nodes/edges
export const transformGraphData = (graphData) => {
    // Check if graphData or its core properties are null/undefined
    if (!graphData || !graphData.nodes || !graphData.edges || !graphData.refs) {
         console.warn("transformGraphData received invalid data:", graphData);
         return { nodes: [], edges: [] };
    }

    const nodes = [];
    const edges = [];
    const nodeMap = new Map(); // To quickly check if target nodes exist

    // Process commit nodes
    graphData.nodes.forEach(node => {
         if (!node || !node.id) return; // Skip invalid nodes
         nodes.push({
             id: node.id,
             data: { label: node.label }, // Label includes sha, author, msg
             position: { x: 0, y: 0 },
             // type assigned later based on ID
         });
          nodeMap.set(node.id, true);
    });

     // Process ref nodes (branches, tags, HEAD)
     graphData.refs.forEach(ref => {
          if (!ref || !ref.name || !ref.target) return; // Skip invalid refs or refs without targets
          // Only add ref node if its target commit node exists
          if (nodeMap.has(ref.target)) {
               nodes.push({
                   id: ref.name, // Use unique name like "branch_main"
                   data: { label: ref.label, type: ref.type }, // Store type in data for custom node
                   position: { x: 0, y: 0 },
                   type: 'ref', // Assign specific type
               });
               nodeMap.set(ref.name, true); // Add ref node to map as well

               // Add edge from ref node to commit node
                edges.push({
                    id: `e-${ref.name}-${ref.target}`,
                    source: ref.name,
                    target: ref.target,
                     // type: 'smoothstep', // Let default edge options handle type
                     style: { // Style ref edges differently
                          stroke: ref.type === 'branch' ? '#0d6efd' : (ref.type === 'tag' ? '#ffc107' : '#198754'), // Use theme colors?
                          strokeWidth: 2,
                     }
                });
          } else {
               console.warn(`Ref '${ref.name}' points to non-existent commit node '${ref.target}'. Skipping ref.`);
          }
     });


    // Process edges between commits
    graphData.edges.forEach(edge => {
         if (!edge || !edge.source || !edge.target) return; // Skip invalid edges
          // Ensure both source and target nodes exist before adding edge
         if (nodeMap.has(edge.source) && nodeMap.has(edge.target)) {
             edges.push({
                 id: `e-${edge.source}-${edge.target}`,
                 source: edge.source,
                 target: edge.target,
                 // type: 'smoothstep', // Let default options handle type
             });
         } else {
              console.warn(`Skipping edge from '${edge.source}' to '${edge.target}' as one or both nodes do not exist.`);
         }
    });


    return { nodes, edges };
};