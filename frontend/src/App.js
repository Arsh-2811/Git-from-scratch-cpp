// src/App.js
import React from 'react';
import { BrowserRouter as Router, Routes, Route, useLocation } from 'react-router-dom';
import { ThemeProvider, CssBaseline } from '@mui/material';
import { AnimatePresence } from 'framer-motion';
import theme from './styles/theme';
import HomePage from './pages/HomePage';
import RepositoryPage from './pages/RepositoryPage';
import NotFoundPage from './pages/NotFoundPage';
import './styles/global.css'; // Import global styles

// Helper to allow AnimatePresence to track route changes
function AnimatedRoutes() {
     const location = useLocation();
     return (
          <AnimatePresence mode="wait"> {/* wait mode ensures exit anim completes first */}
               <Routes location={location} key={location.pathname}> {/* Use location and key */}
                    <Route path="/" element={<HomePage />} />
                    <Route path="/repo/:repoName/*" element={<RepositoryPage />} />
                    <Route path="*" element={<NotFoundPage />} />
               </Routes>
          </AnimatePresence>
     );
}


function App() {
    return (
        <ThemeProvider theme={theme}>
            <CssBaseline /> {/* Normalize CSS and apply background */}
            <Router>
                 <AnimatedRoutes /> {/* Use the animated routes component */}
            </Router>
        </ThemeProvider>
    );
}

export default App;