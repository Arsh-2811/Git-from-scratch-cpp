// src/components/CommitDetailView.js
import React, { useEffect } from 'react';
import { useParams, Link as RouterLink, useNavigate } from 'react-router-dom';
import { Typography, Box, Paper, Skeleton, Chip, Divider, Link, Button, Tooltip } from '@mui/material';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import CodeIcon from '@mui/icons-material/Code';
import useFetchData from '../hooks/useFetchData';
import { getCommitDetails } from '../api/repositoryApi';
import ErrorDisplay from './common/ErrorDisplay';
import LoadingIndicator from './common/LoadingIndicator';
import { formatCommitDate, shortenSha } from '../utils/helpers';

function CommitDetailView() {
    const { repoName, commitSha } = useParams();
    const navigate = useNavigate();
    // Get ref from query params to maintain context when browsing files from commit
    const queryParams = new URLSearchParams(window.location.search);
    const parentRef = queryParams.get('ref') || 'HEAD'; // Ref used to reach this page

    const { data: commit, loading, error, fetchData } = useFetchData(getCommitDetails, [], false);

    useEffect(() => {
        if (repoName && commitSha) {
            console.log(`CommitDetailView Fetching: repo=${repoName}, commit=${commitSha}`);
            fetchData(repoName, commitSha);
        } else {
            console.log(`CommitDetailView Skipping Fetch: repo=${repoName}, commit=${commitSha}`);
        }
    }, [repoName, commitSha, fetchData]);


    if (loading) {
        return <LoadingIndicator message={`Loading commit ${shortenSha(commitSha)}...`} />;
    }

    if (error) {
        return <ErrorDisplay error={error} />;
    }

    if (!commit) {
         return null; // Or a "not found" message if fetch completed with no data
    }

    const authorName = commit.author?.split('<')[0].trim() || 'Unknown Author';
    const commitDate = commit.date ? new Date(parseInt(commit.date.split(' ')[0], 10) * 1000) : null;


    return (
        <Paper variant="outlined" sx={{ p: {xs: 1.5, md: 3} }}>
            {/* Back Button */}
             <Button
                startIcon={<ArrowBackIcon />}
                onClick={() => navigate(`/repo/${repoName}/commits?ref=${encodeURIComponent(parentRef)}`)} // Go back to commit list with original ref
                sx={{ mb: 2 }}
                size="small"
            >
                Back to Commits
            </Button>

             {/* Commit Header */}
            <Typography variant="h5" component="h1" gutterBottom sx={{wordBreak: 'break-word', lineHeight: 1.3}}>
                {commit.message?.split('\n')[0]} {/* First line */}
            </Typography>
            <Box sx={{ mb: 2, display: 'flex', flexWrap: 'wrap', alignItems: 'center', gap: 1 }}>
                 <Chip label={shortenSha(commit.sha)} size="small" sx={{ fontFamily: 'monospace' }} variant="outlined"/>
                 <Typography variant="body2" color="text.secondary">
                     <span style={{fontWeight: 500}}>{authorName}</span> committed on <Tooltip title={commitDate ? commitDate.toLocaleString() : commit.date}><span style={{cursor: 'help'}}>{formatCommitDate(commit.date)}</span></Tooltip>
                 </Typography>
            </Box>

             <Divider sx={{mb: 2}}/>

             {/* Full Commit Message */}
             {commit.message?.includes('\n') && (
                 <Typography variant="body1" sx={{ whiteSpace: 'pre-wrap', fontFamily: 'monospace', backgroundColor: 'action.hover', p: 2, borderRadius: 1, mb: 2, fontSize: '0.9rem' }}>
                     {commit.message.substring(commit.message.indexOf('\n') + 1).trim()} {/* Show rest of message */}
                 </Typography>
             )}

             {/* Commit Details */}
             <Box sx={{display: 'flex', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1, mb: 2, alignItems: 'center'}}>
                  <Box>
                     <Typography variant="body2" display="block"><strong>Commit:</strong> <span style={{fontFamily: 'monospace'}}>{commit.sha}</span></Typography>
                     {commit.parents?.map((parentSha, index) => (
                         <Typography variant="body2" display="block" key={parentSha}>
                              <strong>Parent {commit.parents.length > 1 ? index + 1 : ''}:</strong>{' '}
                              <Link component={RouterLink} to={`/repo/${repoName}/commits/${parentSha}?ref=${parentRef}`} sx={{ fontFamily: 'monospace' }}>
                                   {parentSha}
                              </Link>
                         </Typography>
                     ))}
                      <Typography variant="body2" display="block"><strong>Tree:</strong> <span style={{fontFamily: 'monospace'}}>{commit.tree}</span></Typography>
                  </Box>
                  <Button
                     variant="outlined" size="small"
                     startIcon={<CodeIcon />}
                     component={RouterLink}
                     to={`/repo/${repoName}/tree?ref=${commit.sha}`} // Browse files *at this commit's SHA*
                    >
                      Browse Files
                 </Button>
             </Box>

             <Divider sx={{mt: 3, mb: 3}}/>

             {/* Placeholder for File Changes Diff */}
              <Typography variant="h6" gutterBottom>Changes</Typography>
              <Typography color="text.secondary" sx={{textAlign: 'center', p:3}}>
                   (File differences view not implemented yet)
              </Typography>

        </Paper>
    );
}

export default CommitDetailView;