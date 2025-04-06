// src/components/RefSelector.js
import React, { useState, useEffect } from 'react';
import { Box, FormControl, InputLabel, Select, MenuItem, Chip, Typography, CircularProgress } from '@mui/material';
import AccountTreeIcon from '@mui/icons-material/AccountTree'; // Branch icon
import LocalOfferIcon from '@mui/icons-material/LocalOffer'; // Tag icon
import useFetchData from '../hooks/useFetchData';
import { getBranches, getTags } from '../api/repositoryApi';

function RefSelector({ repoName, currentRef, onRefChange }) {
    const { data: branchesData, loading: loadingBranches, error: errorBranches } = useFetchData(getBranches, [repoName]);
    const { data: tagsData, loading: loadingTags, error: errorTags } = useFetchData(getTags, [repoName]);

    const [refs, setRefs] = useState({ branches: [], tags: [] });

    useEffect(() => {
        setRefs({
            branches: branchesData || [],
            tags: tagsData || []
        });
    }, [branchesData, tagsData]);

    const handleSelectChange = (event) => {
        onRefChange(event.target.value);
    };

    const isLoading = loadingBranches || loadingTags;

    return (
        <FormControl sx={{ m: 1, minWidth: 200 }} size="small">
            <InputLabel id="ref-select-label">Branch/Tag</InputLabel>
            <Select
                labelId="ref-select-label"
                id="ref-select"
                value={currentRef || ''} // Handle case where currentRef might be null/undefined initially
                label="Branch/Tag"
                onChange={handleSelectChange}
                disabled={isLoading}
                renderValue={(selected) => (
                     // Display selected value nicely
                     <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                          {isLoading ? <CircularProgress size={20}/> : (refs.branches.some(b => b.name === selected) ? <AccountTreeIcon fontSize="small" /> : <LocalOfferIcon fontSize="small" />)}
                         <Typography variant="body2">{selected || 'Select Ref...'}</Typography>
                     </Box>
                 )}
            >
                {isLoading && <MenuItem disabled><em>Loading...</em></MenuItem>}
                {errorBranches && <MenuItem disabled><em style={{ color: 'red' }}>Error loading branches</em></MenuItem>}
                {errorTags && <MenuItem disabled><em style={{ color: 'red' }}>Error loading tags</em></MenuItem>}

                <MenuItem disabled><Typography variant="caption" color="text.secondary">Branches</Typography></MenuItem>
                {refs.branches.map((branch) => (
                    <MenuItem key={`branch-${branch.name}`} value={branch.name}>
                        <Box sx={{ display: 'flex', alignItems: 'center', width: '100%' }}>
                             <AccountTreeIcon sx={{ mr: 1, color: 'text.secondary' }} fontSize="small" />
                             <Typography variant="body2" sx={{ flexGrow: 1 }}>{branch.name}</Typography>
                            {branch.isCurrent && <Chip label="Current" size="small" color="primary" sx={{ ml: 1 }}/>}
                        </Box>
                    </MenuItem>
                ))}

                <MenuItem disabled><Typography variant="caption" color="text.secondary">Tags</Typography></MenuItem>
                 {refs.tags.map((tag) => (
                    <MenuItem key={`tag-${tag.name}`} value={tag.name}>
                         <Box sx={{ display: 'flex', alignItems: 'center', width: '100%' }}>
                              <LocalOfferIcon sx={{ mr: 1, color: 'text.secondary' }} fontSize="small" />
                              <Typography variant="body2">{tag.name}</Typography>
                              {/* Optionally show tag type */}
                              {/* {tag.type === 'annotated' && <Chip label="Annotated" size="small" sx={{ ml: 1 }} />} */}
                         </Box>
                    </MenuItem>
                ))}
                {/* Allow selecting specific commit SHAs? Maybe via search/input later */}
            </Select>
        </FormControl>
    );
}

export default RefSelector;