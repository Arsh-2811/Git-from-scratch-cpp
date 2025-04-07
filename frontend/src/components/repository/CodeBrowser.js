// src/components/repository/CodeBrowser.js
import React, { useEffect } from 'react';
import { useNavigate, Link as RouterLink } from 'react-router-dom';
import { Box, List, ListItem, ListItemButton, ListItemIcon, ListItemText, Typography, Skeleton, Breadcrumbs, Link } from '@mui/material';
import FolderIcon from '@mui/icons-material/Folder';
import ArrowBackIcon from '@mui/icons-material/ArrowBack'; // For parent directory
import useFetchData from '../../hooks/useFetchData';
import { getTree } from '../../api/repositoryApi';
import { shortenSha } from '../../utils/helpers';
import ErrorDisplay from '../common/ErrorDisplay';
import AnimatedListItem from '../common/AnimatedListItem';
import FileIcon from './FileIcon';

function CodeBrowser({ repoName, currentRef, path }) {
    const navigate = useNavigate();
    const currentPath = path || '';
    // Fetch needs repoName & currentRef; path is used to construct the request
    const { data: files, loading, error, fetchData } = useFetchData(getTree, [], false);

    useEffect(() => {
        // Fetch only if core identifiers are present
        if (repoName && currentRef !== undefined) {
            console.log(`CodeBrowser Fetching: repo=${repoName}, ref=${currentRef}, path=${currentPath}`);
            fetchData(repoName, currentRef, currentPath);
        } else {
             console.log(`CodeBrowser Skipping Fetch: repo=${repoName}, ref=${currentRef}`);
        }
        // Depend on repoName, currentRef, and the path derived from the route
    }, [repoName, currentRef, currentPath, fetchData]);

    const handleItemClick = (item) => {
        const newPath = currentPath ? `${currentPath}/${item.name}` : item.name;
        // Ensure proper encoding happens in the navigate step or API call
        const encodedNewPath = newPath.split('/').map(encodeURIComponent).join('/');
        const encodedRefParam = encodeURIComponent(currentRef);

        if (item.type === 'tree') {
            navigate(`/repo/${repoName}/tree/${encodedNewPath}?ref=${encodedRefParam}`);
        } else {
            navigate(`/repo/${repoName}/blob/${encodedNewPath}?ref=${encodedRefParam}`);
        }
    };

    const pathSegments = currentPath.split('/').filter(Boolean);
    const breadcrumbs = [
        <Link
            component={RouterLink} underline="hover" color="inherit"
            sx={{fontWeight: 500, display: 'flex', alignItems: 'center'}}
            to={`/repo/${repoName}/tree?ref=${encodeURIComponent(currentRef)}`} key="root"
        >
            {/* Optional Icon */}
            {/* <FolderIcon sx={{ mr: 0.5, fontSize: 'inherit' }} /> */}
            {repoName}
        </Link>,
        ...pathSegments.map((segment, index) => {
            const pathSoFar = pathSegments.slice(0, index + 1).join('/');
            const encodedPathSoFar = pathSoFar.split('/').map(encodeURIComponent).join('/');
            const isLast = index === pathSegments.length - 1;
            return isLast ? (
                <Typography color="text.primary" sx={{fontWeight: 500}} key={encodedPathSoFar}>{segment}</Typography>
            ) : (
                <Link component={RouterLink} underline="hover" color="inherit" to={`/repo/${repoName}/tree/${encodedPathSoFar}?ref=${encodeURIComponent(currentRef)}`} key={encodedPathSoFar}>
                    {segment}
                </Link>
            );
        })
    ];

    return (
        <Box>
            <Breadcrumbs aria-label="breadcrumb" separator="›" sx={{
                 mb: 2, p: 1.5,
                 // bgcolor: alpha(theme.palette.primary.main, 0.05), // Subtle background
                 borderBottom: 1, borderColor: 'divider',
                 // borderRadius: 1,
                 overflowX: 'auto', whiteSpace: 'nowrap' // Handle long paths
                 }}>
                {breadcrumbs}
            </Breadcrumbs>

            {loading && <Box sx={{pl: 1, pr: 1}}>{[...Array(5)].map((_, i) => <Skeleton key={i} variant="rectangular" height={45} sx={{mb: 0.5, borderRadius: 1}} />)}</Box>}
            {error && <ErrorDisplay error={error} />}
            {!loading && !error && files && (
                 <List dense sx={{pl: {xs:0, sm:1}, pr: {xs:0, sm:1}}}>
                      {currentPath && ( // Show ".." link to go up
                           <AnimatedListItem index={-1}>
                               <ListItem disablePadding>
                                   <ListItemButton sx={{borderRadius: 1.5}} onClick={() => {
                                       const parentPath = pathSegments.slice(0, -1).join('/');
                                       const encodedParentPath = parentPath.split('/').map(encodeURIComponent).join('/');
                                       navigate(`/repo/${repoName}/tree${parentPath ? '/' + encodedParentPath : ''}?ref=${encodeURIComponent(currentRef)}`);
                                   }}>
                                       <ListItemIcon sx={{minWidth: '40px'}}><ArrowBackIcon sx={{ color: 'text.secondary' }} /></ListItemIcon>
                                       <ListItemText primary="..." sx={{ fontStyle: 'italic', color: 'text.secondary' }} />
                                   </ListItemButton>
                               </ListItem>
                           </AnimatedListItem>
                      )}
                      {/* Sort: directories first, then files */}
                      {files.sort((a, b) => {
                           if (a.type === b.type) return a.name.localeCompare(b.name);
                           return a.type === 'tree' ? -1 : 1;
                      }).map((item, index) => (
                           <AnimatedListItem key={item.sha + item.name} index={index}>
                               <ListItem disablePadding>
                                   <ListItemButton onClick={() => handleItemClick(item)} sx={{gap: 1.5, borderRadius: 1.5}}>
                                       <ListItemIcon sx={{ minWidth: '30px', display: 'flex', justifyContent: 'center' }}>
                                           {item.type === 'tree'
                                               ? <FolderIcon sx={{ color: 'primary.light' }} /> // Slightly lighter blue for folders
                                               : <FileIcon filename={item.name} />
                                           }
                                       </ListItemIcon>
                                       <ListItemText
                                            primary={<Link component={RouterLink} to="#" onClick={(e) => { e.preventDefault(); handleItemClick(item); }} underline='hover' color="text.primary" sx={{fontWeight: 400}}>{item.name}</Link>}
                                            sx={{ flexGrow: 1, mr: 2 }}
                                       />
                                       {/* Future: Add last commit message here */}
                                       <Typography variant="caption" color="text.secondary" sx={{ fontFamily: 'monospace', minWidth: '60px', textAlign: 'right' }}>
                                            {shortenSha(item.sha)}
                                        </Typography>
                                   </ListItemButton>
                               </ListItem>
                           </AnimatedListItem>
                      ))}
                 </List>
             )}
            {/* Empty State Messages */}
            {!loading && !error && (!files || files.length === 0) && !currentPath && (
                 <Typography sx={{ textAlign: 'center', p: 4, fontStyle: 'italic' }} color="text.secondary">Repository is empty or has no commits.</Typography>
            )}
            {!loading && !error && files && files.length === 0 && currentPath && (
                 <Typography sx={{ textAlign: 'center', p: 4, fontStyle: 'italic' }} color="text.secondary">This directory is empty.</Typography>
            )}
        </Box>
    );
}

export default CodeBrowser;