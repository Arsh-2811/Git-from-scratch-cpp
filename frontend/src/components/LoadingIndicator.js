import React from 'react';
import { Box, CircularProgress, Typography } from '@mui/material';

function LoadingIndicator({ message = "Loading...", size = 40 }) {
    return (
        <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', p: 4, flexDirection: 'column', gap: 2 }}>
            <CircularProgress size={size} />
            <Typography variant="body1" color="text.secondary">{message}</Typography>
        </Box>
    );
}

export default LoadingIndicator;