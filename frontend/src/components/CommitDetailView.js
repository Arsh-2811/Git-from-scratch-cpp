// src/components/CommitDetailView.js
import React, { useEffect } from 'react';
import { useParams } from 'react-router-dom';
import { Typography, Box, Paper, Chip } from '@mui/material';
import useFetchData from '../hooks/useFetchData'; // Assuming you'll use this
import { getCommitDetails } from '../api/repositoryApi'; // Assuming API function exists
import ErrorDisplay from './ErrorDisplay';
import LoadingIndicator from './LoadingIndicator';
import { formatCommitDate, shortenSha } from '../utils/helpers'; // Import helpers

function CommitDetailView() {
    // Call useParams at the top level of this component
    const { repoName, commitSha } = useParams();

    // --- Fetch Commit Data ---
    // Set autoFetch to false, fetch manually in useEffect
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
         // Render nothing or a different loading state if fetch hasn't started/completed yet
         // This prevents rendering placeholder if repoName/commitSha are briefly undefined
         return null;
    }

    // --- Render Commit Details ---
    return (
        <Paper elevation={1} sx={{ p: 3 }}>
            <Typography variant="h5" gutterBottom>
                {commit.message?.split('\n')[0]} {/* First line */}
            </Typography>
            <Box sx={{ mb: 2, display: 'flex', alignItems: 'center', gap: 1 }}>
                 <Chip label={shortenSha(commit.sha)} size="small" sx={{ fontFamily: 'monospace' }} />
                 <Typography variant="body2" color="text.secondary">
                     {commit.author?.split('<')[0].trim() || 'Unknown Author'} committed on {formatCommitDate(commit.date)}
                 </Typography>
            </Box>
            <Typography variant="body1" sx={{ whiteSpace: 'pre-wrap', fontFamily: 'monospace', backgroundColor: '#f5f5f5', p: 2, borderRadius: 1 }}>
                {commit.message}
            </Typography>
             <Box mt={2}>
                 <Typography variant="body2" ><strong>Tree:</strong> {commit.tree}</Typography>
                 {commit.parents?.map((parentSha, index) => (
                     <Typography variant="body2" key={parentSha}><strong>Parent {index + 1}:</strong> {parentSha}</Typography>
                 ))}
             </Box>

            {/* TODO: Add diff view / file changes here later */}

        </Paper>
    );
}

export default CommitDetailView;