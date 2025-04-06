import React from 'react';
import { AppBar, Toolbar, Typography, Box } from '@mui/material';
import { Link } from 'react-router-dom';
import CodeIcon from '@mui/icons-material/Code'; // Example Icon

function Header() {
    return (
        <AppBar position="static">
            <Toolbar>
                <CodeIcon sx={{ mr: 1 }} />
                <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
                    <Link to="/" style={{ color: 'inherit', textDecoration: 'none' }}>
                        MyGit Web UI
                    </Link>
                </Typography>
                {/* Add Login/User info later if needed */}
                {/* <Button color="inherit">Login</Button> */}
            </Toolbar>
        </AppBar>
    );
}

export default Header;