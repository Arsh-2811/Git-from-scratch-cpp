// src/pages/NotFoundPage.js
import React from 'react';
import { Container, Typography, Button, Box } from '@mui/material';
import { Link } from 'react-router-dom';
import Header from '../components/Header';

function NotFoundPage() {
    return (
        <>
            <Header />
            <Container maxWidth="sm" sx={{ textAlign: 'center', mt: 8 }}>
                <Typography variant="h1" component="h1" gutterBottom>
                    404
                </Typography>
                <Typography variant="h5" component="h2" gutterBottom>
                    Oops! Page Not Found.
                </Typography>
                <Typography color="text.secondary" sx={{ mb: 4 }}>
                    The page you are looking for might have been removed, had its name changed,
                    or is temporarily unavailable.
                </Typography>
                <Button variant="contained" component={Link} to="/">
                    Go to Homepage
                </Button>
            </Container>
        </>
    );
}

export default NotFoundPage;