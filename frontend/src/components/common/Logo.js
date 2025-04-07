// src/components/common/Logo.js
import React from 'react';
import { SvgIcon } from '@mui/material';

// Simple placeholder - replace with your actual SVG or Icon
const LogoIcon = (props) => (
  <SvgIcon {...props} viewBox="0 0 24 24">
    <path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z" /> {/* Example path */}
  </SvgIcon>
);


function Logo({ sx = {} }) {
    // Link to home or just display
    return (
        <LogoIcon sx={{ fontSize: 32, color: 'common.white', ...sx }} />
    );
}

export default Logo;