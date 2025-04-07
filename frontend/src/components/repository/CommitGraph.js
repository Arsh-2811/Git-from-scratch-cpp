// src/components/repository/CommitGraph.js
import React, { useEffect, useMemo, useCallback } from 'react';
import { Box, Typography, Paper, Chip, Tooltip, useTheme, alpha } from '@mui/material';
import CommitIcon from '@mui/icons-material/Commit';
import useFetchData from '../../hooks/useFetchData';
import { getCommits } from '../../api/repositoryApi';
import { transformGraphData } from '../../utils/helpers';
import LoadingIndicator from '../common/LoadingIndicator';
import ErrorDisplay from '../common/ErrorDisplay';
import ReactFlow, {
    MiniMap, Controls, Background, // Keep Background import if you want dots/color later
    useNodesState, useEdgesState, MarkerType, Position, Handle
} from 'reactflow';
import 'reactflow/dist/style.css';
import Dagre from '@dagrejs/dagre';
import { motion } from 'framer-motion';

// --- Graph Layout Logic ---
const dagreGraph = new Dagre.graphlib.Graph({ compound: true });
dagreGraph.setDefaultEdgeLabel(() => ({}));
const nodeWidth = 210; // Slightly wider
const nodeHeight = 85; // Increased height substantially for wrapping
const refNodeHeight = 32;

const getLayoutedElements = (nodes, edges, direction = 'TB') => {
    const isHorizontal = direction === 'LR';
    dagreGraph.setGraph({ rankdir: direction, nodesep: 45, ranksep: 85 }); // More spacing

    nodes.forEach((node) => {
        const width = node.type === 'ref' ? (node.data.label.length * 9 + 30) : nodeWidth; // Adjusted ref width estimate
        const height = node.type === 'ref' ? refNodeHeight : nodeHeight;
        dagreGraph.setNode(node.id, { width, height });
    });

    edges.forEach((edge) => { dagreGraph.setEdge(edge.source, edge.target); });
    Dagre.layout(dagreGraph);

    nodes.forEach((node) => {
        const nodeWithPosition = dagreGraph.node(node.id);
        node.targetPosition = isHorizontal ? Position.Left : Position.Top;
        node.sourcePosition = isHorizontal ? Position.Right : Position.Bottom;
        const width = node.type === 'ref' ? (node.data.label.length * 9 + 30) : nodeWidth;
        const height = node.type === 'ref' ? refNodeHeight : nodeHeight;
        node.position = { x: nodeWithPosition.x - width / 2, y: nodeWithPosition.y - height / 2 };
        return node;
    });
    return { nodes, edges };
};
// --- End Layout Logic ---


// --- Custom Node Components ---
const MotionPaper = motion(Paper);

const CommitNode = React.memo(({ data }) => {
    const theme = useTheme();
    const [sha, author, message] = data.label?.split('\n') || ['?', '?', '?'];
    // Show only first line OR allow wrapping - let's allow wrapping
    const fullTooltip = `${sha}\n${author}\n${message || '(No message)'}`;

    return (
        <>
            <Handle type="target" position={Position.Top} style={{ visibility: 'hidden', height: '1px', width: '1px' }} isConnectable={false} /> {/* Make handle smaller */}
            <Tooltip title={<span style={{ whiteSpace: 'pre-line' }}>{fullTooltip}</span>} placement="top" arrow followCursor>
                <MotionPaper
                    elevation={0}
                    variant="outlined"
                    sx={{
                        padding: '12px 15px', // Adjusted padding
                        borderRadius: '8px',
                        borderColor: 'divider',
                        background: theme.palette.background.paper,
                        textAlign: 'left',
                        width: nodeWidth,
                        height: nodeHeight, // Fixed height still needed for layout
                        fontSize: '12px',
                        fontFamily: theme.typography.fontFamily,
                        cursor: 'pointer',
                        overflow: 'hidden', // Keep hidden to respect fixed size
                        display: 'flex',
                        flexDirection: 'column',
                        justifyContent: 'space-around', // Space out content vertically
                        boxSizing: 'border-box',
                        transition: 'border-color 0.2s ease, box-shadow 0.2s ease',
                        '&:hover': {
                            borderColor: theme.palette.primary.light,
                            boxShadow: `0 2px 8px ${alpha(theme.palette.primary.main, 0.15)}`,
                        }
                    }}
                    initial={{ opacity: 0, scale: 0.95 }}
                    animate={{ opacity: 1, scale: 1 }}
                    transition={{ duration: 0.3, ease: 'easeOut' }}
                >
                    <Box sx={{ display: 'flex', alignItems: 'center', mb: 0.5 }}> {/* Reduced bottom margin */}
                         <CommitIcon sx={{ fontSize: 16, color: 'text.secondary', mr: 1 }}/>
                         <Typography variant="body2" component="div" sx={{ fontWeight: 'bold', color: 'primary.main', fontFamily: 'monospace' }}>{sha}</Typography>
                    </Box>
                     {/* Allow Author to wrap if needed, but less likely */}
                    <Typography variant="caption" component="div" sx={{ color: 'text.secondary', whiteSpace: 'normal', overflowWrap: 'break-word', lineHeight: 1.3 }}>{author}</Typography>
                     {/* Allow Message to wrap, limit lines with CSS clamp */}
                    <Typography
                        variant="body2"
                        component="div"
                        title={message} // Add browser tooltip for full message just in case
                        sx={{
                            color: 'text.primary',
                            mt: 0.5,
                            whiteSpace: 'normal', // Allow wrapping
                            overflow: 'hidden', // Required for line clamp
                            textOverflow: 'ellipsis', // Fallback
                            display: '-webkit-box', // Enable line clamp
                            WebkitLineClamp: 2, // Limit to 2 lines
                            WebkitBoxOrient: 'vertical',
                            fontSize: '0.8rem',
                            lineHeight: 1.4, // Adjust line height for wrapped text
                         }}
                     >{message || '(No message)'}</Typography>
                </MotionPaper>
            </Tooltip>
            <Handle type="source" position={Position.Bottom} style={{ visibility: 'hidden', height: '1px', width: '1px' }} isConnectable={false} /> {/* Make handle smaller */}
        </>
    );
});

// RefNode remains largely the same
const RefNode = React.memo(({ data }) => {
    const theme = useTheme();
    const nodeType = data.type;
    const colorMap = { branch: theme.palette.info.main, tag: theme.palette.warning.dark, head: theme.palette.success.main };
    const chipColor = colorMap[nodeType] || theme.palette.grey[500];
    const isHead = nodeType === 'head';

    return (
         <>
             <motion.div initial={{ opacity: 0 }} animate={{ opacity: 1}} transition={{delay: 0.4}}>
                  <Chip
                     label={data.label} size="small"
                     sx={{ fontWeight: 'bold', cursor: 'pointer', border: `1px solid ${chipColor}`, bgcolor: isHead ? chipColor : alpha(chipColor, 0.1), color: isHead ? theme.palette.getContrastText(chipColor) : chipColor, '& .MuiChip-label': { px: 1.5 } }}
                 />
             </motion.div>
             <Handle type="source" position={Position.Bottom} style={{ visibility: 'hidden', height: '1px', width: '1px' }} isConnectable={false}/>
         </>
    );
});
// --- End Custom Nodes ---

function CommitGraph({ repoName, currentRef }) {
    const theme = useTheme();
    const { data: graphDataApi, loading, error, fetchData } = useFetchData(getCommits, [], false);
    const [nodes, setNodes, onNodesChange] = useNodesState([]);
    const [edges, setEdges, onEdgesChange] = useEdgesState([]);

    const nodeTypes = useMemo(() => ({ commit: CommitNode, ref: RefNode }), []);

    useEffect(() => {
        if (repoName && currentRef !== undefined) { fetchData(repoName, currentRef, true); }
    }, [repoName, currentRef, fetchData]);

    const performLayout = useCallback((apiNodes, apiEdges) => {
           const typedNodes = apiNodes.map(n => ({ ...n, type: (n.id.startsWith('branch_') || n.id.startsWith('tag_') || n.id === 'HEAD') ? 'ref' : 'commit', data: { ...n.data, type: (n.id.startsWith('branch_') ? 'branch' : n.id.startsWith('tag_') ? 'tag' : n.id === 'HEAD' ? 'head' : 'commit') } }));
           const nodeMap = new Map(typedNodes.map(n => [n.id, n]));
           const validEdges = apiEdges.filter(e => nodeMap.has(e.source) && nodeMap.has(e.target));
           const { nodes: layoutedNodes, edges: layoutedEdges } = getLayoutedElements(typedNodes, validEdges, 'TB');
           setNodes(layoutedNodes);
           setEdges(layoutedEdges.map(e => {
                return { ...e, type: 'default', animated: false, markerEnd: { type: MarkerType.ArrowClosed, color: theme.palette.text.disabled, width: 12, height: 12 }, style: { stroke: theme.palette.text.disabled, strokeWidth: 1.5 } }; // Use disabled color for edges
           }));
     }, [setNodes, setEdges, theme.palette.text.disabled]); // Updated dependencies

     useEffect(() => {
         if (graphDataApi?.nodes && graphDataApi?.edges) {
             const { nodes: apiNodes, edges: apiEdges } = transformGraphData(graphDataApi);
              if (apiNodes.length > 0) { performLayout(apiNodes, apiEdges); }
              else { setNodes([]); setEdges([]); }
         } else if (!loading) { setNodes([]); setEdges([]); }
     }, [graphDataApi, loading, performLayout, setNodes, setEdges]);

    const onNodeClick = useCallback((event, node) => { console.log('Clicked node:', node); }, []);

    return (
        <Box sx={{ height: '75vh', border: 1, borderColor: 'divider', mt: 1, position: 'relative', borderRadius: theme.shape.borderRadius, overflow: 'hidden', bgcolor: theme.palette.background.default }}> {/* Use theme default background */}
            {loading && <Box sx={{position: 'absolute', /* ... */ zIndex: 10}}><LoadingIndicator message="Loading graph..." /></Box>}
            {error && <Box sx={{position: 'absolute', /* ... */ zIndex: 10}}><ErrorDisplay error={error} /></Box>}
             <ReactFlow
                nodes={nodes} edges={edges} onNodesChange={onNodesChange} onEdgesChange={onEdgesChange}
                nodeTypes={nodeTypes} onNodeClick={onNodeClick}
                fitView fitViewOptions={{ padding: 0.4, maxZoom: 1.2 }} // More padding
                proOptions={{ hideAttribution: true }}
                nodesDraggable={false} nodesConnectable={false} elementsSelectable={false}
                panOnDrag={true} zoomOnScroll={true} zoomOnPinch={true} zoomOnDoubleClick={true}
                minZoom={0.1}
             >
                 <Controls showInteractive={false} position="top-right"/>
                 <MiniMap nodeStrokeWidth={3} nodeColor={(n) => n.style?.background || '#ddd'} zoomable pannable position="bottom-right"/>
                 {/* Removed Background component entirely for a plain background */}
                 {/* <Background variant="lines" gap={24} size={0.4} color={theme.palette.divider} /> */}
             </ReactFlow>
            {!loading && !error && nodes.length === 0 && (
                 <Box sx={{position: 'absolute', /* ... */ display:'flex', alignItems:'center', justifyContent:'center'}}>
                      <Typography color="text.secondary">No commit history to graph.</Typography>
                 </Box>
            )}
        </Box>
    );
}

export default CommitGraph;