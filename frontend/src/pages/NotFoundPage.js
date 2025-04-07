// src/pages/NotFoundPage.js
import React from 'react';
import { Container, Typography, Button, Box, Paper } from '@mui/material'; // Added Paper
import { Link as RouterLink } from 'react-router-dom';
import Header from '../components/layout/Header';
import PageTransition from '../components/common/PageTransition';
import ErrorOutlineIcon from '@mui/icons-material/ErrorOutline'; // Add an icon

function NotFoundPage() {
    return (
        <PageTransition>
            <Header />
            <Container maxWidth="sm" sx={{ textAlign: 'center', mt: 8 }}>
                 <Paper sx={{ p: 4 }}>
                     <ErrorOutlineIcon sx={{ fontSize: 60, color: 'text.secondary', mb: 2 }} />
                    <Typography variant="h3" component="h1" gutterBottom sx={{fontWeight: 700}}>
                        404
                    </Typography>
                    <Typography variant="h5" component="h2" color="text.secondary" gutterBottom>
                        Oops! Page Not Found
                    </Typography>
                    <Typography color="text.secondary" sx={{ mb: 4 }}>
                        The page you are looking for might have been removed or is temporarily unavailable.
                    </Typography>
                    <Button variant="contained" component={RouterLink} to="/">
                        Go Back Home
                    </Button>
                </Paper>
            </Container>
        </PageTransition>
    );
}

export default NotFoundPage;