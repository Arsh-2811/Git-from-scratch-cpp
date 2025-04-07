// src/views/GraphView.js
import React from 'react';
import CommitGraph from '../components/repository/CommitGraph';
import { Typography, Box } from '@mui/material';

function GraphView({ repoName, currentRef }) {
     return (
          <Box>
              <Typography variant="h6" gutterBottom sx={{ pl: 1, mb: 2 }}>Commit Graph</Typography>
              <CommitGraph repoName={repoName} currentRef={currentRef} />
          </Box>
     );
}

export default GraphView;