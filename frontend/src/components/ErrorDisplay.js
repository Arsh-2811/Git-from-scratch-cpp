import React from 'react';
import { Alert, AlertTitle } from '@mui/material';

function ErrorDisplay({ error }) {
    let title = 'Error';
    let message = 'An unexpected error occurred.';

    if (error instanceof Error) {
        message = error.message;
        if (error.status) {
            title = `Error ${error.status}`;
        }
    } else if (typeof error === 'string') {
        message = error;
    }

    return (
        <Alert severity="error" sx={{ m: 2 }}>
            <AlertTitle>{title}</AlertTitle>
            {message}
        </Alert>
    );
}

export default ErrorDisplay;