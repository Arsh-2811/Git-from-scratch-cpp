// src/views/TagsView.js
import React from 'react';
import TagList from '../components/repository/TagList';
import { Typography, Box } from '@mui/material';


function TagsView({ repoName }) {
    return (
         <Box>
             <Typography variant="h6" gutterBottom sx={{ pl: 1, mb: 2 }}>Tags</Typography>
             <TagList repoName={repoName} />
         </Box>
    );
}

export default TagsView;