// src/components/common/LoadingIndicator.js
import React from 'react';
import { Box, CircularProgress, Typography } from '@mui/material';

// Could add more fancy loaders later (e.g., Skeleton loaders matching content)
function LoadingIndicator({ message = "Loading...", size = 40, sx = {} }) { // Added sx prop
    return (
        <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', p: 4, flexDirection: 'column', gap: 2, ...sx }}>
            <CircularProgress size={size} />
            {message && <Typography variant="body1" color="text.secondary">{message}</Typography>}
        </Box>
    );
}

export default LoadingIndicator;