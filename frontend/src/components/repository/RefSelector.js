// src/components/repository/RefSelector.js
import React, { useState, useEffect } from 'react';
import { Box, FormControl, InputLabel, Select, MenuItem, Chip, Typography, CircularProgress, Tooltip, Fade } from '@mui/material';
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import LocalOfferIcon from '@mui/icons-material/LocalOffer';
import CommitIcon from '@mui/icons-material/Commit'; // For specific commit indication
import useFetchData from '../../hooks/useFetchData';
import { getBranches, getTags } from '../../api/repositoryApi';

function RefSelector({ repoName, currentRef, onRefChange }) {
    const { data: branchesData, loading: loadingBranches, error: errorBranches } = useFetchData(getBranches, [repoName]);
    const { data: tagsData, loading: loadingTags, error: errorTags } = useFetchData(getTags, [repoName]);

    const [refs, setRefs] = useState({ branches: [], tags: [], isCommit: false });

    useEffect(() => {
         const branches = branchesData || [];
         const tags = tagsData || [];
         // Check if currentRef is a known branch or tag
         const isBranch = branches.some(b => b.name === currentRef);
         const isTag = tags.some(t => t.name === currentRef);
         // Assume it's a commit if not HEAD, branch, or tag (basic check)
         const isCommit = currentRef !== 'HEAD' && !isBranch && !isTag;

        setRefs({ branches, tags, isCommit });
    }, [branchesData, tagsData, currentRef]);

    const handleSelectChange = (event) => {
        onRefChange(event.target.value);
    };

    const isLoading = loadingBranches || loadingTags;
    const iconSize = "1.1em";

     const renderValue = (selected) => {
         let IconComponent = CommitIcon; // Default to commit if unknown
         if (selected === 'HEAD') IconComponent = AccountTreeIcon; // Or a specific HEAD icon
         else if (refs.branches.some(b => b.name === selected)) IconComponent = AccountTreeIcon;
         else if (refs.tags.some(t => t.name === selected)) IconComponent = LocalOfferIcon;

         return (
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, overflow: 'hidden' }}>
                   {isLoading ? <CircularProgress size={18} sx={{mr: 0.5}}/> : <IconComponent sx={{fontSize: iconSize}}/>}
                  <Typography variant="body2" sx={{textOverflow: 'ellipsis', overflow:'hidden', whiteSpace:'nowrap'}}>{selected || 'Select Ref...'}</Typography>
              </Box>
          );
      };


    return (
        <FormControl sx={{ m: 1, minWidth: 200, maxWidth: 300 }} size="small">
            <InputLabel id="ref-select-label">Ref</InputLabel>
            <Select
                labelId="ref-select-label"
                id="ref-select"
                value={currentRef || ''}
                label="Ref"
                onChange={handleSelectChange}
                disabled={isLoading}
                renderValue={renderValue}
                MenuProps={{
                    PaperProps: {
                        style: {
                            maxHeight: 400, // Limit dropdown height
                        },
                    },
                 }}
            >
                {isLoading && <MenuItem disabled><em>Loading refs...</em></MenuItem>}
                {errorBranches && <MenuItem disabled><em style={{ color: 'red' }}>Error loading branches</em></MenuItem>}
                {errorTags && <MenuItem disabled><em style={{ color: 'red' }}>Error loading tags</em></MenuItem>}

                 {/* Option for current specific commit if not a branch/tag */}
                  {refs.isCommit && (
                      <MenuItem value={currentRef} sx={{ fontStyle: 'italic', backgroundColor: 'action.selected' }}>
                         <Box sx={{ display: 'flex', alignItems: 'center', width: '100%', gap: 1 }}>
                             <CommitIcon sx={{ fontSize: iconSize, color: 'text.secondary' }} />
                             <Typography variant="body2" sx={{ fontFamily: 'monospace' }}>{currentRef.substring(0,10)}...</Typography>
                         </Box>
                      </MenuItem>
                  )}

                 {/* Branches */}
                {refs.branches.length > 0 && <MenuItem disabled divider><Typography variant="caption" color="text.secondary">Branches</Typography></MenuItem>}
                {refs.branches.map((branch) => (
                    <MenuItem key={`branch-${branch.name}`} value={branch.name}>
                        <Box sx={{ display: 'flex', alignItems: 'center', width: '100%', gap: 1 }}>
                             <AccountTreeIcon sx={{ fontSize: iconSize, color: 'text.secondary' }} />
                             <Typography variant="body2" sx={{ flexGrow: 1, fontWeight: branch.name === currentRef ? 500 : 400 }}>{branch.name}</Typography>
                            {branch.isCurrent && <Chip label="HEAD" size="small" color="success" variant="outlined" sx={{ ml: 1 }}/>}
                        </Box>
                    </MenuItem>
                ))}

                 {/* Tags */}
                 {refs.tags.length > 0 && <MenuItem disabled divider><Typography variant="caption" color="text.secondary">Tags</Typography></MenuItem>}
                 {refs.tags.map((tag) => (
                    <MenuItem key={`tag-${tag.name}`} value={tag.name}>
                         <Box sx={{ display: 'flex', alignItems: 'center', width: '100%', gap: 1 }}>
                              <LocalOfferIcon sx={{ fontSize: iconSize, color: 'text.secondary' }} />
                              <Typography variant="body2" sx={{fontWeight: tag.name === currentRef ? 500 : 400}}>{tag.name}</Typography>
                         </Box>
                    </MenuItem>
                ))}
            </Select>
        </FormControl>
    );
}

export default RefSelector;