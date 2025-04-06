// src/App.js
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import { ThemeProvider, CssBaseline } from '@mui/material';
import theme from './styles/theme';
import HomePage from './pages/HomePage';
import RepositoryPage from './pages/RepositoryPage';
import NotFoundPage from './pages/NotFoundPage';
import './styles/global.css'; // Import global styles

function App() {
    return (
        <ThemeProvider theme={theme}>
            <CssBaseline /> {/* Normalize CSS and apply background */}
            <Router>
                <Routes>
                    <Route path="/" element={<HomePage />} />
                    {/* Route to capture repo name and subsequent path */}
                    <Route path="/repo/:repoName/*" element={<RepositoryPage />} />
                    <Route path="*" element={<NotFoundPage />} />
                </Routes>
            </Router>
        </ThemeProvider>
    );
}

export default App;