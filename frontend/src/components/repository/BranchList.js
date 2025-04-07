// src/components/repository/BranchList.js
import React, { useEffect } from 'react';
import { Box, List, ListItem, ListItemIcon, ListItemText, Typography, Skeleton, Chip } from '@mui/material';
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import useFetchData from '../../hooks/useFetchData';
import { getBranches } from '../../api/repositoryApi';
import { shortenSha } from '../../utils/helpers';
import ErrorDisplay from '../common/ErrorDisplay';
import AnimatedListItem from '../common/AnimatedListItem'; // Use animation

function BranchList({ repoName }) {
    const { data: branches, loading, error, fetchData } = useFetchData(getBranches, [], false);

    useEffect(() => {
         if(repoName) {
              fetchData(repoName);
         }
    }, [repoName, fetchData]);

   return (
        <Box>
            {loading && repoName && <Box>{[...Array(3)].map((_, i) => <Skeleton key={i} variant="text" height={50} />)}</Box>}
            {error && <ErrorDisplay error={error} />}
            {!loading && !error && branches && repoName && (
                 <List>
                      {branches.map((branch, index) => (
                           <AnimatedListItem key={branch.name} index={index}>
                               <ListItem
                                   divider
                                   secondaryAction={
                                       branch.isCurrent && <Chip label="Current" color="primary" size="small" variant="outlined"/>
                                   }
                                >
                                   <ListItemIcon sx={{minWidth: '40px'}}>
                                        <AccountTreeIcon color={branch.isCurrent ? "primary" : "action"} />
                                   </ListItemIcon>
                                   <ListItemText
                                        primary={branch.name}
                                        secondary={branch.sha ? shortenSha(branch.sha) : 'N/A'}
                                        primaryTypographyProps={{ fontWeight: 500 }}
                                        secondaryTypographyProps={{ fontFamily: 'monospace'}}
                                    />
                               </ListItem>
                           </AnimatedListItem>
                      ))}
                 </List>
             )}
             {!loading && !error && (!branches || branches.length === 0) && repoName && (
                 <Typography sx={{ textAlign: 'center', p: 3 }} color="text.secondary">No branches found.</Typography>
             )}
        </Box>
   );
}

export default BranchList;