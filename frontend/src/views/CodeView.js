// src/views/CodeView.js
import React from 'react';
import CodeBrowser from '../components/repository/CodeBrowser';
import FileViewer from '../components/repository/FileViewer';

// This view decides whether to show the browser or the viewer based on the pathSuffix
function CodeView({ repoName, currentRef, pathSuffix }) {
    // Determine view type based on the *first* segment after /repoName/
    const viewType = pathSuffix?.split('/')[0]; // 'tree', 'blob', or potentially undefined/empty

    // Extract the actual path *after* 'tree/' or 'blob/'
    let relativePath = '';
    if (viewType === 'tree') {
        relativePath = pathSuffix?.substring(pathSuffix.indexOf('/') + 1) ?? '';
         // Handle case where pathSuffix is exactly "tree" -> root
         if(pathSuffix === 'tree') relativePath = '';
    } else if (viewType === 'blob') {
        relativePath = pathSuffix?.substring(pathSuffix.indexOf('/') + 1) ?? '';
    }

    const showBrowser = viewType === 'tree';
    const showViewer = viewType === 'blob';

    return (
        <>
            {/* Pass the extracted relativePath */}
            {showBrowser && <CodeBrowser repoName={repoName} currentRef={currentRef} path={relativePath} />}
            {showViewer && <FileViewer repoName={repoName} currentRef={currentRef} filePath={relativePath} />}
             {/* Handle cases where viewType is invalid? */}
        </>
    );
}

export default CodeView;