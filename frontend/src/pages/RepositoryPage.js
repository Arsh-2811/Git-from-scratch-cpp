// src/pages/RepositoryPage.js
import React from 'react';
import { useParams, useNavigate, useLocation, Link as RouterLink, Routes, Route, Navigate } from 'react-router-dom';
import { Box, Container, Typography, Paper, Tabs, Tab, Link } from '@mui/material';
import HomeIcon from '@mui/icons-material/Home';
import CodeIcon from '@mui/icons-material/Code';
import HistoryIcon from '@mui/icons-material/History';
import GraphIcon from '@mui/icons-material/Timeline';
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import LocalOfferIcon from '@mui/icons-material/LocalOffer';
import { AnimatePresence, motion } from 'framer-motion'; // Import AnimatePresence and motion

import Header from '../components/layout/Header';
import RefSelector from '../components/repository/RefSelector';
import LoadingIndicator from '../components/common/LoadingIndicator';
import PageTransition from '../components/common/PageTransition'; // For overall page load

// Import the View components
import CodeView from '../views/CodeView';
import CommitsView from '../views/CommitsView';
import GraphView from '../views/GraphView';
import BranchesView from '../views/BranchesView';
import TagsView from '../views/TagsView';

// --- Animation Variants for View Transitions ---
const viewVariants = {
    hidden: { opacity: 0, y: 15 },
    visible: { opacity: 1, y: 0 },
    exit: { opacity: 0, y: -15 },
};

const viewTransition = {
     type: "tween", // Smooth tween
     duration: 0.3, // Faster transition
     ease: "easeInOut",
};
// --- End Animation Variants ---


function RepositoryPage() {
    const { repoName, '*': pathSuffix } = useParams();
    const location = useLocation(); // Use location for AnimatePresence key
    const navigate = useNavigate();

    const queryParams = new URLSearchParams(location.search);
    const currentRef = queryParams.get('ref') || 'HEAD';

     const pathSegments = pathSuffix?.split('/') || [];
     const currentView = pathSegments[0] || 'tree';

     const viewToTabMap = {
         'tree': 'code', 'blob': 'code', 'commits': 'commits',
         'graph': 'graph', 'branches': 'branches', 'tags': 'tags',
     };
     const currentTab = viewToTabMap[currentView] || 'code';

    const handleRefChange = (newRef) => {
         queryParams.set('ref', newRef);
         const targetView = currentView === 'blob' ? 'tree' : currentView;
         const basePath = `/repo/${repoName}/${targetView || 'tree'}`;
         navigate(`${basePath}?${queryParams.toString()}`, { replace: true });
    };

    const handleTabChange = (event, newValue) => {
        const tabToViewMap = {
            'code': 'tree', 'commits': 'commits', 'graph': 'graph',
            'branches': 'branches', 'tags': 'tags',
        };
        const targetView = tabToViewMap[newValue] || 'tree';
         navigate(`/repo/${repoName}/${targetView}?ref=${encodeURIComponent(currentRef)}`);
    };

    if (!repoName) {
        return <PageTransition><Header /><Container><LoadingIndicator /></Container></PageTransition>;
    }

    return (
        // Overall Page Transition (optional, applied on initial load/exit)
        // <PageTransition>
        <> {/* Use Fragment if PageTransition is removed or applied elsewhere */}
            <Header />
            <Container maxWidth="xl" sx={{ mt: 3, mb: 4 }}>
                {/* Header Area */}
                <Paper /* ... props ... */ sx={{ p: 2, mb: 3, display: 'flex', flexWrap: 'wrap', alignItems: 'center', justifyContent: 'space-between', gap: 2 }}>
                    <Typography /* ... props ... */>
                        <HomeIcon sx={{ verticalAlign: 'bottom', mr: 1, fontSize: '1.2em'}}/>
                        <Link component={RouterLink} to="/" underline="hover" color="inherit">Repositories</Link>
                        {' / '}
                        <span style={{ fontWeight: 600 }}>{repoName}</span>
                    </Typography>
                    <RefSelector repoName={repoName} currentRef={currentRef} onRefChange={handleRefChange} />
                </Paper>

                {/* Tabs */}
                <Box sx={{ borderBottom: 1, borderColor: 'divider', bgcolor: 'background.paper', borderTopLeftRadius: (theme) => theme.shape.borderRadius, borderTopRightRadius: (theme) => theme.shape.borderRadius }}>
                    <Tabs value={currentTab} onChange={handleTabChange} aria-label="Repository Sections" variant="scrollable" scrollButtons="auto" sx={{px: 2}}>
                        {/* ... Tab components ... */}
                         <Tab icon={<CodeIcon />} iconPosition="start" label="Code" value="code" />
                         <Tab icon={<HistoryIcon />} iconPosition="start" label="Commits" value="commits" />
                         <Tab icon={<GraphIcon />} iconPosition="start" label="Graph" value="graph" />
                         <Tab icon={<AccountTreeIcon />} iconPosition="start" label="Branches" value="branches" />
                         <Tab icon={<LocalOfferIcon />} iconPosition="start" label="Tags" value="tags" />
                    </Tabs>
                </Box>

                {/* Main Content Area with Animation */}
                <Paper sx={{ p: { xs: 1.5, sm: 2.5 }, minHeight: '60vh', borderTopLeftRadius: 0, borderTopRightRadius: 0, overflow: 'hidden' }}> {/* Add overflow hidden */}
                    {/* Wrap Routes with AnimatePresence */}
                    <AnimatePresence mode="wait">
                         {/* Use motion.div inside Routes to apply animations */}
                         {/* Keying the motion.div with location.pathname ensures animation on route change */}
                        <motion.div
                            key={location.pathname} // Key based on path
                            variants={viewVariants}
                            initial="hidden"
                            animate="visible"
                            exit="exit"
                            transition={viewTransition}
                        >
                            <Routes location={location}> {/* Pass location to Routes */}
                                {/* Routes remain the same */}
                                <Route path="tree/*" element={<CodeView repoName={repoName} currentRef={currentRef} pathSuffix={pathSuffix} />} />
                                <Route path="tree" element={<CodeView repoName={repoName} currentRef={currentRef} pathSuffix="tree" />} />
                                <Route path="blob/*" element={<CodeView repoName={repoName} currentRef={currentRef} pathSuffix={pathSuffix} />} />
                                <Route path="commits/*" element={<CommitsView repoName={repoName} currentRef={currentRef} />} /> {/* CommitsView handles nested sha */}
                                <Route path="graph" element={<GraphView repoName={repoName} currentRef={currentRef} />} />
                                <Route path="branches" element={<BranchesView repoName={repoName} />} />
                                <Route path="tags" element={<TagsView repoName={repoName} />} />
                                <Route index element={<Navigate to={`tree?ref=${encodeURIComponent(currentRef)}`} replace />} />
                                <Route path="*" element={<Typography color="error" sx={{p:3, textAlign:'center'}}>Section not found.</Typography>} />
                            </Routes>
                         </motion.div>
                    </AnimatePresence>
                </Paper>
            </Container>
        {/* </PageTransition> */}
        </>
    );
}

export default RepositoryPage;