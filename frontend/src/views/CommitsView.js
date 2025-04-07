// src/views/CommitsView.js
import React from 'react';
import { Routes, Route } from 'react-router-dom';
import CommitList from '../components/repository/CommitList';
import CommitDetailView from '../components/CommitDetailView'; // Use the detail component

// Handles routing within the commits section
function CommitsView({ repoName, currentRef }) { // Removed pathSuffix, let Routes handle it

    return (
        // Use nested Routes to handle /commits and /commits/:sha
        <Routes>
             {/* Route for the specific commit detail view */}
            <Route path=":commitSha" element={<CommitDetailView />} />
             {/* Default route for the commit list */}
            <Route index element={<CommitList repoName={repoName} currentRef={currentRef} />} />
        </Routes>
    );
}

export default CommitsView;