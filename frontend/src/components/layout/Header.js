// src/components/layout/Header.js
import React from 'react';
import { AppBar, Toolbar, Typography, IconButton } from '@mui/material';
import { Link as RouterLink } from 'react-router-dom';
// import Logo from '../common/Logo'; // Use your logo component
import { FaGithubAlt } from "react-icons/fa"; // Example icon

function Header() {
    return (
        <AppBar position="sticky" sx={{ zIndex: (theme) => theme.zIndex.drawer + 1 }}>
            <Toolbar>
                 <IconButton
                     component={RouterLink} to="/"
                     edge="start"
                     color="inherit"
                     aria-label="home"
                     sx={{ mr: 1 }}
                    >
                     {/* <Logo /> */}
                     <FaGithubAlt size="1.5em"/> {/* Placeholder Icon */}
                 </IconButton>

                <Typography variant="h6" component="div" sx={{ flexGrow: 1, fontWeight: 600 }}>
                    <RouterLink to="/" style={{ color: 'inherit', textDecoration: 'none' }}>
                        MyGit UI
                    </RouterLink>
                </Typography>

                {/* Placeholder for future actions */}
                {/* <Button color="inherit">Login</Button> */}
            </Toolbar>
        </AppBar>
    );
}

export default Header;