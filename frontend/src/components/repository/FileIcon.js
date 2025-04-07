// src/components/repository/FileIcon.js
import React from 'react';
import { FaFileCode, FaFileImage, FaFilePdf, FaFileArchive, FaFileAlt, FaFileCsv, FaFileWord, FaFileExcel } from 'react-icons/fa';
import { DiJava, DiJavascript1, DiPython, DiHtml5, DiCss3Full, DiReact, DiMarkdown, DiGo, DiRuby } from 'react-icons/di'; // Added Go, Ruby
import { SiCplusplus, SiC, H } from 'react-icons/si'; // Use Si for C/C++
import { Box } from '@mui/material';

// Expanded mapping
const iconMap = {
    js: <DiJavascript1 color="#f0db4f" />,
    jsx: <DiReact color="#61dafb" />,
    ts: <DiJavascript1 color="#3178c6" />, // Often TypeScript uses JS icon or specific TS one
    tsx: <DiReact color="#61dafb" />, // React with TS
    java: <DiJava color="#ea2d2e" />,
    py: <DiPython color="#3572a5" />,
    html: <DiHtml5 color="#e34c26" />,
    css: <DiCss3Full color="#264de4" />,
    scss: <DiCss3Full color="#c69" />, // SCSS
    md: <DiMarkdown color="#555" />,
    json: <FaFileCode color="#f0db4f" />, // Generic code often works for JSON
    c: <SiC color="#a8b9cc"/>,
    h: <SiC color="#a8b9cc"/>, // C Header
    cpp: <SiCplusplus color="#00599c" />,
    hpp: <SiCplusplus color="#00599c" />,
    cxx: <SiCplusplus color="#00599c" />,
    go: <DiGo color="#00add8" />,
    rb: <DiRuby color="#cc342d" />,
    sh: <FaFileCode color="#89e051" />, // Shell script
    yaml: <FaFileAlt color="#cb171e" />, // Generic often used for YAML/YML
    yml: <FaFileAlt color="#cb171e" />,
    png: <FaFileImage color="#ff6f61" />,
    jpg: <FaFileImage color="#ff6f61" />,
    jpeg: <FaFileImage color="#ff6f61" />,
    gif: <FaFileImage color="#ff6f61" />,
    svg: <FaFileImage color="#ffb13b" />, // SVG specific
    pdf: <FaFilePdf color="#e40000" />,
    zip: <FaFileArchive color="#95a5a6" />,
    rar: <FaFileArchive color="#95a5a6" />,
    gz: <FaFileArchive color="#95a5a6" />,
    tar: <FaFileArchive color="#95a5a6" />,
    csv: <FaFileCsv color="#27ae60" />,
    doc: <FaFileWord color="#2b579a" />,
    docx: <FaFileWord color="#2b579a" />,
    xls: <FaFileExcel color="#217346" />,
    xlsx: <FaFileExcel color="#217346" />,
    txt: <FaFileAlt color="#89e051" />, // Generic text
    // Add more specific types
    code: <FaFileCode color="#555" />, // Generic code
    text: <FaFileAlt color="#555" />, // Generic text
};

const defaultIcon = <FaFileAlt color="#555" />;

function FileIcon({ filename }) {
    const extension = filename?.split('.').pop()?.toLowerCase();
    let icon = defaultIcon;
    const lowerFilename = filename?.toLowerCase() || '';

    // Handle specific filenames first
    if (lowerFilename === 'dockerfile') icon = iconMap.code; // Could use a Docker icon
    else if (lowerFilename.endsWith('.gitignore')) icon = iconMap.text;
    else if (lowerFilename.endsWith('.npmrc')) icon = iconMap.text;
    else if (lowerFilename === 'license') icon = iconMap.text;
    else if (lowerFilename.startsWith('readme')) icon = iconMap.md; // Treat README variations as markdown
    // Then check extensions
    else if (extension) {
        icon = iconMap[extension] || iconMap.text;
    }

    return <Box sx={{ display: 'inline-flex', verticalAlign: 'middle', fontSize: '1.25em' }}>{icon}</Box>;
}

export default FileIcon;