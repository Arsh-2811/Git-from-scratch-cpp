// src/components/repository/TagList.js
import React, { useEffect } from 'react';
import { Box, List, ListItem, ListItemIcon, ListItemText, Typography, Skeleton, Link, Tooltip } from '@mui/material';
import LocalOfferIcon from '@mui/icons-material/LocalOffer';
import { Link as RouterLink } from 'react-router-dom';
import useFetchData from '../../hooks/useFetchData';
import { getTags } from '../../api/repositoryApi';
import { shortenSha } from '../../utils/helpers';
import ErrorDisplay from '../common/ErrorDisplay';
import AnimatedListItem from '../common/AnimatedListItem';

function TagList({ repoName }) {
    const { data: tags, loading, error, fetchData } = useFetchData(getTags, [], false);

    useEffect(() => {
        if(repoName) {
             fetchData(repoName);
        }
   }, [repoName, fetchData]);

    return (
        <Box>
           {loading && repoName && <Box>{[...Array(3)].map((_, i) => <Skeleton key={i} variant="text" height={50} />)}</Box>}
           {error && <ErrorDisplay error={error} />}
           {!loading && !error && tags && repoName && (
                <List>
                     {tags.map((tag, index) => (
                         <AnimatedListItem key={tag.name} index={index}>
                             <ListItem
                                 divider
                                 secondaryAction={
                                      <Tooltip title={`View code at tag ${tag.name}`}>
                                          <Link
                                               component={RouterLink}
                                               to={`/repo/${repoName}/tree?ref=${encodeURIComponent(tag.name)}`} // Link to tree view for this tag
                                               sx={{ fontFamily: 'monospace', color: 'text.secondary', '&:hover': { color: 'primary.main' }}}
                                           >
                                              {shortenSha(tag.targetSha) || shortenSha(tag.sha)} {/* Show target preferred */}
                                          </Link>
                                      </Tooltip>
                                 }
                                >
                                 <ListItemIcon sx={{minWidth: '40px'}}>
                                     <LocalOfferIcon color="action" />
                                 </ListItemIcon>
                                 <ListItemText
                                      primary={tag.name}
                                      secondary={`Type: ${tag.type || '?'}`}
                                      primaryTypographyProps={{ fontWeight: 500 }}
                                 />
                             </ListItem>
                         </AnimatedListItem>
                     ))}
                </List>
            )}
             {!loading && !error && (!tags || tags.length === 0) && repoName && (
                 <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No tags found.</Typography>
             )}
        </Box>
    );
}

export default TagList;