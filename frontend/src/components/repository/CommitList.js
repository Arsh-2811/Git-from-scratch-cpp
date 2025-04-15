// src/components/repository/CommitList.js
import React, { useEffect } from 'react';
import { Link as RouterLink } from 'react-router-dom';
import { Box, List, ListItem, Typography, Skeleton, Link, IconButton, Tooltip, Avatar, Fade, Paper } from '@mui/material';
import CodeIcon from '@mui/icons-material/Code';
import useFetchData from '../../hooks/useFetchData';
import { getCommits } from '../../api/repositoryApi';
import { shortenSha } from '../../utils/helpers';
import ErrorDisplay from '../common/ErrorDisplay';
import { formatDistanceToNowStrict } from 'date-fns'; // For relative time

// Basic avatar generator from name initials
const stringToColor = (string) => {
  let hash = 0; let i;
  for (i = 0; i < string.length; i += 1) { hash = string.charCodeAt(i) + ((hash << 5) - hash); }
  let color = '#';
  for (i = 0; i < 3; i += 1) { const value = (hash >> (i * 8)) & 0xff; color += `00${value.toString(16)}`.slice(-2); }
  return color;
}

const stringAvatar = (name) => {
  if (!name || name.indexOf('<') === 0) return { sx: { bgcolor: '#bdbdbd', width: 32, height: 32 }, children: '?' };
  const nameParts = name.split('<')[0].trim().split(' ');
  return {
    sx: { bgcolor: stringToColor(name), width: 32, height: 32, fontSize: '0.8rem' },
    children: `${nameParts[0][0]}${nameParts.length > 1 ? nameParts[nameParts.length - 1][0] : ''}`.toUpperCase(),
  };
}

function CommitList({ repoName, currentRef }) {
    // Add pagination state later: const [page, setPage] = useState({ skip: 0, limit: 25 });
    const { data: commits, loading, error, fetchData } = useFetchData(getCommits, [], false);

    useEffect(() => {
        if (repoName && currentRef !== undefined) {
            // Add pagination params: fetchData(repoName, currentRef, false, page.limit, page.skip);
             fetchData(repoName, currentRef, false /* graph */);
        }
    // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [repoName, currentRef, fetchData /*, page.skip, page.limit */]); // Add page state to deps later

    const renderCommitItem = (commit) => {
        const commitTimestamp = commit.date ? parseInt(commit.date.split(' ')[0], 10) * 1000 : null;
        const commitDate = commitTimestamp ? new Date(commitTimestamp) : null;
        const relativeTime = commitDate ? formatDistanceToNowStrict(commitDate, { addSuffix: true }) : 'unknown time';
        const authorName = commit.author?.split('<')[0].trim() || 'Unknown Author';

         return (
            <ListItem
                key={commit.sha} alignItems="flex-start" divider
                sx={{ py: 1.5, '&:last-child': { borderBottom: 0 } }}
            >
                 <Avatar {...stringAvatar(authorName)} sx={{ mr: 2, mt: 0.5 }} />
                 <Box sx={{ display: 'flex', justifyContent: 'space-between', width: '100%', flexWrap:'wrap', gap: 1 }}>
                     <Box sx={{ flexGrow: 1, minWidth: '200px' }}>
                         <Typography variant="body1" component="div" sx={{ fontWeight: 500, mb: 0.5, wordBreak: 'break-word', lineHeight: 1.4 }}>
                             {commit.message.split('\n')[0]}
                         </Typography>
                         <Typography variant="body2" color="text.secondary">
                              <span style={{ fontWeight: '500' }}>{authorName}</span> committed <Tooltip title={commitDate ? commitDate.toLocaleString() : commit.date}><span style={{cursor: 'help'}}>{relativeTime}</span></Tooltip>
                         </Typography>
                     </Box>
                     <Box sx={{ textAlign: 'right', minWidth: '100px', alignSelf:'center', display: 'flex', alignItems: 'center' }}>
                          <Tooltip title={`View commit ${commit.sha}`}>
                              <Link component={RouterLink} to={`/repo/${repoName}/commits/${commit.sha}?ref=${currentRef}`} sx={{ fontWeight: 'medium', fontFamily: 'monospace', mr: 1, color: 'text.secondary', '&:hover': { color: 'primary.main' }}}>
                                  {shortenSha(commit.sha)}
                              </Link>
                          </Tooltip>
                          <Tooltip title="Browse files at this commit">
                               <IconButton size="small" component={RouterLink} to={`/repo/${repoName}/tree?ref=${commit.sha}`} sx={{p: 0.5}}>
                                    <CodeIcon fontSize="small" />
                               </IconButton>
                          </Tooltip>
                     </Box>
                 </Box>
            </ListItem>
        );
    }

    return (
         <Box>
             {/* Add loading skeletons matching the item structure */}
            {loading && <Box sx={{pl: 1, pr: 1}}>{[...Array(7)].map((_, i) => <Skeleton key={i} variant="rectangular" height={65} sx={{ mb: 0.5, borderRadius: 1 }}/>)}</Box>}
            {error && <ErrorDisplay error={error} />}
            {!loading && !error && commits && (
                commits.length > 0 ? (
                    // Add paper wrapper for visual grouping
                    <Paper variant="outlined">
                         <List sx={{ padding: 0 }}>
                             {commits.map((commit, index) => (
                                 <Fade in={true} key={commit.sha} timeout={300 + index * 50}>
                                      {renderCommitItem(commit)}
                                 </Fade>
                             ))}
                         </List>
                    </Paper>
                ) : (
                    <Typography sx={{ textAlign: 'center', p: 3, fontStyle: 'italic' }} color="text.secondary">
                        No commits found for this reference.
                    </Typography>
                )
            )}
             {/* Add Pagination Controls Here Later */}
         </Box>
    );
}

export default CommitList;