// src/components/common/ErrorDisplay.js
import React from 'react';
import { Alert, AlertTitle, Typography } from '@mui/material';
import ReportProblemIcon from '@mui/icons-material/ReportProblem';

function ErrorDisplay({ error, sx = {} }) { // Added sx prop
    let title = 'Error';
    let message = 'An unexpected error occurred.';

    if (error instanceof Error) {
        // Skip AbortError messages
        if (error.name === 'AbortError') {
            return null; // Don't display anything for aborted requests
        }
        message = error.message;
        if (error.status) {
            title = `Error ${error.status}`;
        }
    } else if (typeof error === 'string') {
        message = error;
    }

    return (
        <Alert severity="error" sx={{ m: 2, ...sx }} icon={<ReportProblemIcon fontSize="inherit" />}>
            <AlertTitle>{title}</AlertTitle>
            <Typography variant="body2">{message}</Typography>
        </Alert>
    );
}

export default ErrorDisplay;