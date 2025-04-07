// src/views/BranchesView.js
import React from 'react';
import BranchList from '../components/repository/BranchList';
import { Typography, Box } from '@mui/material'; // Import Box


function BranchesView({ repoName }) {
    return (
         // Add Box for consistent padding/margin if needed
         <Box>
             <Typography variant="h6" gutterBottom sx={{ pl: 1, mb: 2 }}>Branches</Typography>
             <BranchList repoName={repoName} />
         </Box>
    );
}

export default BranchesView;