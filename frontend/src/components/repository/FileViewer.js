// src/components/repository/FileViewer.js
import React, { useEffect, useMemo } from 'react';
import { Box, Paper, Typography, Skeleton, Breadcrumbs, Link } from '@mui/material';
import { Link as RouterLink } from 'react-router-dom'; // For breadcrumbs
import useFetchData from '../../hooks/useFetchData';
import { getBlobContent } from '../../api/repositoryApi';
import { getFileLanguage } from '../../utils/helpers';
import LoadingIndicator from '../common/LoadingIndicator';
import ErrorDisplay from '../common/ErrorDisplay';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { atomDark } from 'react-syntax-highlighter/dist/esm/styles/prism'; // Example: Different style


function FileViewer({ repoName, currentRef, filePath }) {
    const { data: content, loading, error, fetchData } = useFetchData(getBlobContent, [], false);
    const language = useMemo(() => getFileLanguage(filePath), [filePath]);

    useEffect(() => {
        if (repoName && currentRef !== undefined && filePath) {
            console.log(`FileViewer Fetching: repo=${repoName}, ref=${currentRef}, path=${filePath}`);
            fetchData(repoName, currentRef, filePath);
        } else {
             console.log(`FileViewer Skipping Fetch: repo=${repoName}, ref=${currentRef}, path=${filePath}`);
        }
    }, [repoName, currentRef, filePath, fetchData]);

    // Breadcrumbs logic (similar to CodeBrowser)
     const pathSegments = filePath.split('/').filter(Boolean);
     const fileName = pathSegments.pop() || ''; // Get the filename
     const directoryPath = pathSegments.join('/'); // Path to the directory
     const encodedDirectoryPath = directoryPath.split('/').map(encodeURIComponent).join('/');

     const breadcrumbs = [
        <Link component={RouterLink} underline="hover" color="inherit" sx={{fontWeight: 500}} to={`/repo/${repoName}/tree?ref=${encodeURIComponent(currentRef)}`} key="root">
            {repoName}
        </Link>,
         // Add links for parent directories
        ...pathSegments.map((segment, index) => {
            const pathSoFar = pathSegments.slice(0, index + 1).join('/');
            const encodedPathSoFar = pathSoFar.split('/').map(encodeURIComponent).join('/');
            return (
                <Link component={RouterLink} underline="hover" color="inherit" to={`/repo/${repoName}/tree/${encodedPathSoFar}?ref=${encodeURIComponent(currentRef)}`} key={encodedPathSoFar}>
                    {segment}
                </Link>
            );
        }),
         // Current file name (not a link)
         <Typography color="text.primary" sx={{fontWeight: 500}} key={fileName}>{fileName}</Typography>
    ];


    return (
         <Box>
             {/* Breadcrumbs */}
              <Breadcrumbs aria-label="breadcrumb" separator="›" sx={{
                  mb: 2, p: 1.5,
                  borderBottom: 1, borderColor: 'divider',
                  overflowX: 'auto', whiteSpace: 'nowrap'
                 }}>
                 {breadcrumbs}
             </Breadcrumbs>

            {/* Loading State */}
             {loading && <Skeleton variant="rectangular" animation="wave" height={400} sx={{borderRadius: 1}} />}
             {/* Error State */}
             {error && error.name !== 'AbortError' && <ErrorDisplay error={error} sx={{m: 0}}/>}

            {/* Content Display */}
             {!loading && !error && content !== null && (
                 <Paper variant="outlined" sx={{ maxHeight: '75vh', overflow: 'auto', // Outlined paper looks nice for code
                     '&::-webkit-scrollbar': { width: '6px', height: '6px' }, /* Slim scrollbar */
                     '&::-webkit-scrollbar-thumb': { background: '#ccc', borderRadius: '3px' },
                 }}>
                     <SyntaxHighlighter
                        language={language}
                        style={atomDark} // Use a different style maybe
                        showLineNumbers={true}
                        wrapLines={true}
                        lineNumberStyle={{ color: '#aaa', fontSize: '0.8em', marginRight: '10px' }}
                        customStyle={{ margin: 0, padding: '16px', fontSize: '0.875rem', background: 'transparent !important' }} // Ensure background is handled by Paper
                    >
                         {content || ''}
                     </SyntaxHighlighter>
                 </Paper>
             )}

            {/* Empty/Failed State */}
              {!loading && error?.name !== 'AbortError' && content === null && (
                 <Typography sx={{ textAlign: 'center', p: 4, fontStyle: 'italic' }} color="text.secondary">Could not load file content.</Typography>
             )}
         </Box>
    );
}

export default FileViewer;