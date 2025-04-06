// src/pages/RepositoryPage.js
import React, { useState, useEffect, useMemo } from 'react';
import { useParams, useNavigate, useLocation, Link as RouterLink, Routes, Route, Navigate } from 'react-router-dom';
// MUI Imports - ADDED MISSING ONES
import {
    Box, Container, Typography, Paper, Tabs, Tab, Breadcrumbs, Link,
    Skeleton, IconButton, Tooltip, List, ListItem, ListItemButton,
    ListItemIcon, ListItemText, Chip
} from '@mui/material';
import HomeIcon from '@mui/icons-material/Home';
import CodeIcon from '@mui/icons-material/Code';
// import CommitIcon from '@mui/icons-material/Commit'; // Not used directly in this file's JSX
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import LocalOfferIcon from '@mui/icons-material/LocalOffer';
import DescriptionIcon from '@mui/icons-material/Description';
import FolderIcon from '@mui/icons-material/Folder';
import HistoryIcon from '@mui/icons-material/History';
// import VisibilityIcon from '@mui/icons-material/Visibility'; // Not used directly
import GraphIcon from '@mui/icons-material/Timeline';

import Header from '../components/Header';
import RefSelector from '../components/RefSelector';
import LoadingIndicator from '../components/LoadingIndicator';
import ErrorDisplay from '../components/ErrorDisplay';
import useFetchData from '../hooks/useFetchData';
import { getTree, getBlobContent, getCommits, getBranches, getTags } from '../api/repositoryApi';
import { shortenSha, formatCommitDate, getFileLanguage, transformGraphData } from '../utils/helpers';
// import { formatDistanceToNow } from 'date-fns'; // Not used directly

// Syntax Highlighter
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { dracula } from 'react-syntax-highlighter/dist/esm/styles/prism';

// React Flow - Imports should now work with v11
import ReactFlow, { MiniMap, Controls, Background, useNodesState, useEdgesState, MarkerType } from 'reactflow';
import 'reactflow/dist/style.css'; // Correct path for v11+

import Dagre from '@dagrejs/dagre'; // For graph layout

import CommitDetailView from '../components/CommitDetailView';

// --- Main Repository Page Component ---
function RepositoryPage() {
    const { repoName, '*': pathSuffix } = useParams();
    const location = useLocation();
    const navigate = useNavigate();

    // Extract ref from query params, default to 'HEAD'
    const queryParams = new URLSearchParams(location.search);
    const currentRef = queryParams.get('ref') || 'HEAD';

    // Determine the current active tab based on the path suffix
    let currentTab = 'code'; // Default tab
    if (pathSuffix?.startsWith('commits')) currentTab = 'commits';
    if (pathSuffix?.startsWith('branches')) currentTab = 'branches';
    if (pathSuffix?.startsWith('tags')) currentTab = 'tags';
    if (pathSuffix?.startsWith('graph')) currentTab = 'graph';
    // Adjust logic slightly: if the path is exactly 'tree' or 'blob', it's still the code tab visually
    if (pathSuffix === 'tree' || pathSuffix?.startsWith('tree/') || pathSuffix === 'blob' || pathSuffix?.startsWith('blob/')) {
        currentTab = 'code';
    }


    const handleRefChange = (newRef) => {
         queryParams.set('ref', newRef);
         const basePath = location.pathname.split('/').slice(0, 4).join('/');
         const currentViewType = basePath.split('/')[3];
         let targetPath = location.pathname;
         // If not currently in tree or blob view, default to tree view when changing ref
         if (!['tree', 'blob'].includes(currentViewType)) {
              targetPath = `/repo/${repoName}/tree`;
         }
         navigate(`${targetPath}?${queryParams.toString()}`, { replace: true });
    };

    const handleTabChange = (event, newValue) => {
         let tabPath = '';
         if (newValue === 'code') tabPath = 'tree';
         if (newValue === 'commits') tabPath = 'commits';
         if (newValue === 'branches') tabPath = 'branches';
         if (newValue === 'tags') tabPath = 'tags';
         if (newValue === 'graph') tabPath = 'graph';
         navigate(`/repo/${repoName}/${tabPath}?ref=${encodeURIComponent(currentRef)}`);
    };

    // --- ADD GUARD: Don't render content until repoName is available ---
    if (!repoName) {
        return (
            <>
                <Header />
                <Container maxWidth="xl" sx={{ mt: 3 }}>
                    <LoadingIndicator message="Loading repository..." />
                </Container>
            </>
        );
    }
    // --- END GUARD ---


    return (
        <>
            <Header />
            <Container maxWidth="xl" sx={{ mt: 3 }}>
                {/* Repository Header Area */}
                <Paper elevation={1} sx={{ p: 2, mb: 3, display: 'flex', flexWrap: 'wrap', alignItems: 'center', justifyContent: 'space-between', gap: 2 }}>
                    <Typography variant="h5" component="h1" sx={{ overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                         <HomeIcon sx={{ verticalAlign: 'middle', mr: 1 }}/>
                         <Link component={RouterLink} to="/" underline="hover" color="inherit">Repositories</Link>
                         {' / '}
                         {repoName}
                     </Typography>
                     <RefSelector repoName={repoName} currentRef={currentRef} onRefChange={handleRefChange} />
                </Paper>

                {/* Tabs for Navigation */}
                <Box sx={{ borderBottom: 1, borderColor: 'divider', mb: 3 }}>
                     <Tabs value={currentTab} onChange={handleTabChange} aria-label="Repository Sections" variant="scrollable" scrollButtons="auto">
                         {/* ... Tabs ... */}
                          <Tab icon={<CodeIcon />} iconPosition="start" label="Code" value="code" />
                          <Tab icon={<HistoryIcon />} iconPosition="start" label="Commits" value="commits" />
                          <Tab icon={<GraphIcon />} iconPosition="start" label="Graph" value="graph" />
                          <Tab icon={<AccountTreeIcon />} iconPosition="start" label="Branches" value="branches" />
                          <Tab icon={<LocalOfferIcon />} iconPosition="start" label="Tags" value="tags" />
                     </Tabs>
                </Box>

                {/* Content Area based on Route */}
                 <Paper elevation={2} sx={{ p: 2, minHeight: '50vh' }}>
                     <Routes>
                         {/* --- Updated path extraction --- */}
                         <Route
                             path="tree/*"
                             element={<CodeBrowser repoName={repoName} currentRef={currentRef} path={pathSuffix?.substring(pathSuffix.indexOf('/') + 1) ?? ''} />}
                         />
                          <Route
                             path="tree"
                             element={<CodeBrowser repoName={repoName} currentRef={currentRef} path={''} />}
                         />
                         <Route
                             path="blob/*"
                             element={<FileViewer repoName={repoName} currentRef={currentRef} filePath={pathSuffix?.substring(pathSuffix.indexOf('/') + 1) ?? ''} />}
                          />
                          {/* --- End Updated path extraction --- */}

                         <Route path="commits" element={<CommitList repoName={repoName} currentRef={currentRef} />} />
                         <Route path="commits/:commitSha" element={<CommitDetailView />} />
                         <Route path="graph" element={<CommitGraph repoName={repoName} currentRef={currentRef} />} />
                         <Route path="branches" element={<BranchList repoName={repoName} />} />
                         <Route path="tags" element={<TagList repoName={repoName} />} />
                         <Route index element={<Navigate to={`tree?ref=${encodeURIComponent(currentRef)}`} replace />} />
                         <Route path="*" element={<Typography color="error">View not found within repository.</Typography>} />
                     </Routes>
                 </Paper>
            </Container>
        </>
    );
}

// --- Sub Components (CodeBrowser, FileViewer, etc.) ---

// --- ADD GUARD in CodeBrowser useEffect ---
function CodeBrowser({ repoName, currentRef, path }) {
    const navigate = useNavigate();
    const currentPath = path || '';
    const { data: files, loading, error, fetchData } = useFetchData(getTree, [], false); // Set autoFetch to false

    useEffect(() => {
        // Only fetch if repoName and currentRef are valid
        if (repoName && currentRef) {
             console.log(`CodeBrowser Fetching: repo=${repoName}, ref=${currentRef}, path=${currentPath}`); // Debug log
             fetchData(repoName, currentRef, currentPath);
        } else {
             console.log(`CodeBrowser Skipping Fetch: repo=${repoName}, ref=${currentRef}`); // Debug log
        }
    }, [repoName, currentRef, currentPath, fetchData]); // Keep dependencies

    // ... rest of CodeBrowser component ...
     // Breadcrumbs logic ...
     const pathSegments = currentPath.split('/').filter(Boolean);
     const breadcrumbs = [
         <Link component={RouterLink} underline="hover" color="inherit" to={`/repo/${repoName}/tree?ref=${encodeURIComponent(currentRef)}`} key="root">
             {repoName}
         </Link>,
         ...pathSegments.map((segment, index) => {
             const pathSoFar = pathSegments.slice(0, index + 1).join('/');
             const isLast = index === pathSegments.length - 1;
             return isLast ? (
                 <Typography color="text.primary" key={pathSoFar}>{segment}</Typography>
             ) : (
                 <Link component={RouterLink} underline="hover" color="inherit" to={`/repo/${repoName}/tree/${encodeURIComponent(pathSoFar)}?ref=${encodeURIComponent(currentRef)}`} key={pathSoFar}>
                     {segment}
                 </Link>
             );
         })
     ];

     const handleItemClick = (item) => {
         const newPath = currentPath ? `${currentPath}/${item.name}` : item.name;
         if (item.type === 'tree') {
             // Use relative navigation if possible, or construct full path carefully
             navigate(`/repo/${repoName}/tree/${encodeURIComponent(newPath)}?ref=${encodeURIComponent(currentRef)}`);
         } else { // blob
             navigate(`/repo/${repoName}/blob/${encodeURIComponent(newPath)}?ref=${encodeURIComponent(currentRef)}`);
         }
     };


     return (
         <Box>
             <Breadcrumbs aria-label="breadcrumb" sx={{ mb: 2, p: 1, backgroundColor: 'rgba(0, 0, 0, 0.03)', borderRadius: 1 }}>
                 {breadcrumbs}
             </Breadcrumbs>
             {/* Display loading only if repoName/currentRef are valid */}
             {loading && repoName && currentRef && <Box>{[...Array(5)].map((_, i) => <Skeleton key={i} variant="text" height={40} />)}</Box>}
             {error && <ErrorDisplay error={error} />}
             {!loading && !error && files && repoName && currentRef && (
                 <List dense>
                    {/* ... ListItems ... */}
                      {currentPath && (
                          <ListItemButton onClick={() => {
                              const parentPath = pathSegments.slice(0, -1).join('/');
                              navigate(`/repo/${repoName}/tree${parentPath ? '/' + encodeURIComponent(parentPath) : ''}?ref=${encodeURIComponent(currentRef)}`);
                          }}>
                              <ListItemIcon><FolderIcon sx={{ color: 'text.secondary' }} /></ListItemIcon>
                              <ListItemText primary="..." sx={{ fontStyle: 'italic' }} />
                          </ListItemButton>
                      )}
                      {files.sort((a, b) => {
                           if (a.type === b.type) return a.name.localeCompare(b.name);
                           return a.type === 'tree' ? -1 : 1;
                      }).map((item) => (
                         <ListItem key={item.sha + item.name} disablePadding divider>
                             <ListItemButton onClick={() => handleItemClick(item)}>
                                 <ListItemIcon>
                                     {item.type === 'tree' ? <FolderIcon sx={{ color: 'primary.main' }} /> : <DescriptionIcon sx={{ color: 'text.secondary' }} />}
                                 </ListItemIcon>
                                 <ListItemText primary={item.name} />
                                  <Typography variant="caption" color="text.secondary" sx={{ mr: 2 }}>{shortenSha(item.sha)}</Typography>
                                  <Typography variant="caption" color="text.secondary" >{item.mode}</Typography>
                             </ListItemButton>
                         </ListItem>
                     ))}
                 </List>
             )}
              {/* ... Empty state messages ... */}
               {!loading && !error && (!files || files.length === 0) && !currentPath && repoName && currentRef && (
                 <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">Empty repository</Typography>
             )}
              {!loading && !error && files && files.length === 0 && currentPath && repoName && currentRef && (
                 <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">Empty directory</Typography>
             )}
         </Box>
     );

}

// --- ADD GUARD in FileViewer useEffect ---
function FileViewer({ repoName, currentRef, filePath }) {
    // Manage state directly IF NOT using useFetchData hook for this specific case
    // const [content, setContent] = useState(null);
    // const [loading, setLoading] = useState(false);
    // const [error, setError] = useState(null);
    // OR continue using useFetchData if modified to handle signal (see step 2)

    // If using useFetchData, ensure it correctly handles the signal passing and state setting
    const { data: content, loading, error, fetchData } = useFetchData(getBlobContent, [], false);
    const language = useMemo(() => getFileLanguage(filePath), [filePath]);

    useEffect(() => {
        // --- AbortController Logic ---
        const controller = new AbortController();
        const signal = controller.signal;
        // --- End AbortController Logic ---

        // Only fetch if repoName, currentRef, and filePath are valid
        if (repoName && currentRef && filePath) {
            console.log(`FileViewer Fetching: repo=${repoName}, ref=${currentRef}, path=${filePath}`);
            // Pass the signal in an options object (modify fetchData/getBlobContent)
            fetchData(repoName, currentRef, filePath, { signal });
        } else {
            console.log(`FileViewer Skipping Fetch: repo=${repoName}, ref=${currentRef}, path=${filePath}`);
        }

        // --- Cleanup Function ---
        return () => {
            console.log(`FileViewer Cleanup: Aborting fetch for path=${filePath}`);
            controller.abort(); // Abort the fetch request if it's still in progress
        };
        // --- End Cleanup Function ---

    // Ensure fetchData is stable or handle its potential changes
    }, [repoName, currentRef, filePath, fetchData]); // Keep dependencies


    return (
        <Box sx={{ mt: 2 }}>
            {/* Display loading only if fetching */}
            {loading && <Skeleton variant="rectangular" height={300} />}
            {error && error.name !== 'AbortError' && <ErrorDisplay error={error} />} {/* Don't show AbortError */}
            {!loading && !error && content !== null && (
                 <Paper elevation={2} sx={{ maxHeight: '70vh', overflow: 'auto' }}>
                     <SyntaxHighlighter
                        language={language}
                        style={dracula}
                        showLineNumbers={true}
                        wrapLines={true}
                        customStyle={{ margin: 0, fontSize: '0.85rem' }}
                    >
                         {content || ''}
                     </SyntaxHighlighter>
                 </Paper>
            )}
             {/* Added condition to not show 'could not load' if loading or aborted */}
             {!loading && error?.name !== 'AbortError' && content === null && (
                <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">Could not load file content.</Typography>
            )}
        </Box>
    );
}


// ... CommitList, CommitGraph, BranchList, TagList remain the same ...
 // CommitList Component Logic
 function CommitList({ repoName, currentRef }) {
      // TODO: Add Pagination state and logic
     const { data: commits, loading, error, fetchData } = useFetchData(getCommits, [], false); // autoFetch=false

     useEffect(() => {
          if(repoName && currentRef) {
               fetchData(repoName, currentRef, false /* graph */);
          }
     }, [repoName, currentRef, fetchData]);


     return (
          <Box>
             {loading && repoName && currentRef && <Box>{[...Array(7)].map((_, i) => <Skeleton key={i} variant="text" height={60} sx={{ mb: 1 }}/>)}</Box>}
             {error && <ErrorDisplay error={error} />}
             {!loading && !error && commits && repoName && currentRef && (
                  <List>
                      {commits.map(commit => (
                          <ListItem key={commit.sha} alignItems="flex-start" divider>
                             <Box sx={{ display: 'flex', justifyContent: 'space-between', width: '100%' }}>
                                 <Box>
                                     <Typography variant="body1" component="div" sx={{ fontWeight: 'medium', mb: 0.5 }}>
                                         {commit.message.split('\n')[0]} {/* First line of message */}
                                     </Typography>
                                     <Typography variant="body2" color="text.secondary">
                                          {/* Assuming author format: "Name <email> timestamp zone" */}
                                          {commit.author?.split('<')[0].trim() || 'Unknown Author'} committed {formatCommitDate(commit.date)}
                                     </Typography>
                                 </Box>
                                 <Box sx={{ textAlign: 'right', minWidth: '100px' }}>
                                      <Tooltip title={`View commit ${commit.sha}`}>
                                          {/* Link to commit detail page later */}
                                          <Link component={RouterLink} to={`/repo/${repoName}/commits/${commit.sha}?ref=${currentRef}`} sx={{ fontWeight: 'medium', fontFamily: 'monospace' }}>
                                              {shortenSha(commit.sha)}
                                          </Link>
                                      </Tooltip>
                                      {/* Add browse files button */}
                                      <Tooltip title="Browse files at this commit">
                                           <IconButton size="small" component={RouterLink} to={`/repo/${repoName}/tree?ref=${commit.sha}`}>
                                                <CodeIcon fontSize="small" />
                                           </IconButton>
                                      </Tooltip>
                                 </Box>
                             </Box>
                          </ListItem>
                      ))}
                  </List>
              )}
              {!loading && !error && (!commits || commits.length === 0) && repoName && currentRef && (
                  <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No commits found for this reference.</Typography>
              )}
              {/* Add Pagination Controls */}
          </Box>
     );
 }

 // CommitGraph Component Logic
 const dagreGraph = new Dagre.graphlib.Graph();
 dagreGraph.setDefaultEdgeLabel(() => ({}));
 const nodeWidth = 172;
 const nodeHeight = 50; // Adjust based on label size

 const getLayoutedElements = (nodes, edges, direction = 'TB') => {
   // ... layout logic ...
    const isHorizontal = direction === 'LR';
   dagreGraph.setGraph({ rankdir: direction });

   nodes.forEach((node) => {
     dagreGraph.setNode(node.id, { width: nodeWidth, height: nodeHeight });
   });

   edges.forEach((edge) => {
     dagreGraph.setEdge(edge.source, edge.target);
   });

   Dagre.layout(dagreGraph);

   nodes.forEach((node) => {
     const nodeWithPosition = dagreGraph.node(node.id);
     node.targetPosition = isHorizontal ? 'left' : 'top';
     node.sourcePosition = isHorizontal ? 'right' : 'bottom';

     node.position = {
       x: nodeWithPosition.x - nodeWidth / 2,
       y: nodeWithPosition.y - nodeHeight / 2,
     };

     return node;
   });

   return { nodes, edges };
 };

 function CommitGraph({ repoName, currentRef }) {
     const { data: graphDataApi, loading, error, fetchData } = useFetchData(getCommits, [], false); // autoFetch=false
     const [nodes, setNodes, onNodesChange] = useNodesState([]);
     const [edges, setEdges, onEdgesChange] = useEdgesState([]);

      useEffect(() => {
           if(repoName && currentRef) {
                fetchData(repoName, currentRef, true /* graph */);
           }
      }, [repoName, currentRef, fetchData]);


      useEffect(() => {
          if (graphDataApi) {
              const { nodes: apiNodes, edges: apiEdges } = transformGraphData(graphDataApi);
              const { nodes: layoutedNodes, edges: layoutedEdges } = getLayoutedElements(apiNodes, apiEdges, 'TB');
              setNodes(layoutedNodes);
              setEdges(layoutedEdges);
          } else {
               setNodes([]);
               setEdges([]);
          }
      }, [graphDataApi, setNodes, setEdges]);

     return (
         <Box sx={{ height: '70vh', border: '1px solid #eee', mt: 1, backgroundColor: '#fff' }}>
              {loading && repoName && currentRef && <LoadingIndicator message="Loading graph..." />}
              {error && <ErrorDisplay error={error} />}
              {!loading && !error && graphDataApi && repoName && currentRef && (
                  <ReactFlow
                     // ... props ...
                      nodes={nodes}
                     edges={edges}
                     onNodesChange={onNodesChange}
                     onEdgesChange={onEdgesChange}
                     fitView
                     proOptions={{ hideAttribution: true }}
                     nodesDraggable={true}
                     nodesConnectable={false}
                     defaultEdgeOptions={{
                          markerEnd: { type: MarkerType.ArrowClosed, color: '#aaa' },
                          style: { stroke: '#aaa' }
                     }}
                  >
                     <Controls />
                     <MiniMap nodeStrokeWidth={3} zoomable pannable />
                     <Background variant="dots" gap={12} size={1} />
                 </ReactFlow>
              )}
               {!loading && !error && (!graphDataApi || !graphDataApi.nodes || graphDataApi.nodes.length === 0) && repoName && currentRef && (
                  <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No commit history to graph.</Typography>
              )}
         </Box>
     );
 }

 // BranchList Component Logic
 function BranchList({ repoName }) {
      const { data: branches, loading, error, fetchData } = useFetchData(getBranches, [], false); // autoFetch=false

      useEffect(() => {
           if(repoName) {
                fetchData(repoName);
           }
      }, [repoName, fetchData]);

     return (
          <Box>
             {loading && repoName && <Box>{[...Array(3)].map((_, i) => <Skeleton key={i} variant="text" height={40} />)}</Box>}
             {error && <ErrorDisplay error={error} />}
             {!loading && !error && branches && repoName && (
                  <List>
                       {branches.map(branch => (
                           <ListItem key={branch.name} divider>
                              <ListItemIcon><AccountTreeIcon color={branch.isCurrent ? "primary" : "action"} /></ListItemIcon>
                              <ListItemText primary={branch.name} secondary={shortenSha(branch.sha)} />
                               {branch.isCurrent && <Chip label="Current" color="primary" size="small" />}
                           </ListItem>
                       ))}
                  </List>
              )}
              {!loading && !error && (!branches || branches.length === 0) && repoName && (
                  <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No branches found.</Typography>
              )}
          </Box>
     );
 }

 // TagList Component Logic
 function TagList({ repoName }) {
      const { data: tags, loading, error, fetchData } = useFetchData(getTags, [], false); // autoFetch=false

      useEffect(() => {
           if(repoName) {
                fetchData(repoName);
           }
      }, [repoName, fetchData]);

      return (
          <Box>
             {loading && repoName && <Box>{[...Array(3)].map((_, i) => <Skeleton key={i} variant="text" height={40} />)}</Box>}
             {error && <ErrorDisplay error={error} />}
             {!loading && !error && tags && repoName && (
                  <List>
                       {tags.map(tag => (
                           <ListItem key={tag.name} divider>
                              <ListItemIcon><LocalOfferIcon color="action" /></ListItemIcon>
                              <ListItemText
                                  primary={tag.name}
                                  secondary={`Type: ${tag.type}, Points to: ${tag.targetType || '?'} ${shortenSha(tag.targetSha)}`}
                                 />
                           </ListItem>
                       ))}
                  </List>
              )}
              {!loading && !error && (!tags || tags.length === 0) && repoName && (
                  <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No tags found.</Typography>
              )}
          </Box>
      );
 }


export default RepositoryPage;