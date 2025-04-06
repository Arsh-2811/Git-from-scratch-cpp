// src/pages/HomePage.js
import React from 'react';
import { Link as RouterLink } from 'react-router-dom';
import { Container, Typography, List, ListItem, ListItemButton, ListItemIcon, ListItemText, Paper, Box, Divider } from '@mui/material';
import FolderZipIcon from '@mui/icons-material/FolderZip';
import useFetchData from '../hooks/useFetchData';
import { getAllRepositories } from '../api/repositoryApi';
import LoadingIndicator from '../components/LoadingIndicator';
import ErrorDisplay from '../components/ErrorDisplay';

function HomePage() {
    const { data: repoNames, loading, error } = useFetchData(getAllRepositories);

    return (
        <Container maxWidth="md" sx={{ mt: 4 }}>
            <Paper elevation={3} sx={{ p: 3 }}>
                <Typography variant="h4" gutterBottom component="h1" sx={{ textAlign: 'center', mb: 3 }}>
                    MyGit Repositories
                </Typography>
                <Divider sx={{ mb: 3 }} />

                {loading && <LoadingIndicator />}
                {error && <ErrorDisplay error={error} />}
                {!loading && !error && (
                    repoNames && repoNames.length > 0 ? (
                        <List>
                            {repoNames.map((repoName) => (
                                <ListItem key={repoName} disablePadding divider>
                                    <ListItemButton component={RouterLink} to={`/repo/${repoName}/tree`}>
                                        <ListItemIcon>
                                            <FolderZipIcon color="primary" />
                                        </ListItemIcon>
                                        <ListItemText
                                            primary={repoName}
                                            primaryTypographyProps={{ fontWeight: 'medium', color: 'text.primary' }}
                                        />
                                    </ListItemButton>
                                </ListItem>
                            ))}
                        </List>
                    ) : (
                        <Typography variant="body1" color="text.secondary" sx={{ textAlign: 'center', mt: 2 }}>
                            No repositories found.
                        </Typography>
                    )
                )}
                 {/* Add button to create repo later */}
                 {/* <Box sx={{ display: 'flex', justifyContent: 'center', mt: 3 }}>
                     <Button variant="contained" startIcon={<AddIcon />}>New Repository</Button>
                 </Box> */}
            </Paper>
        </Container>
    );
}

export default HomePage;