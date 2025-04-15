// src/pages/HomePage.js
import React from 'react';
import { Link as RouterLink } from 'react-router-dom';
import { Container, Typography, Grid, Card, CardActionArea, CardContent, Box, Paper } from '@mui/material'; // Added Paper
import { FaCodeBranch } from "react-icons/fa";
import useFetchData from '../hooks/useFetchData';
import { getAllRepositories } from '../api/repositoryApi';
import LoadingIndicator from '../components/common/LoadingIndicator';
import ErrorDisplay from '../components/common/ErrorDisplay';
import PageTransition from '../components/common/PageTransition';
import AnimatedListItem from '../components/common/AnimatedListItem';
import Header from '../components/layout/Header'; // Adjusted path

function HomePage() {
    const { data: repoNames, loading, error } = useFetchData(getAllRepositories);

    return (
        <PageTransition>
             <Header />
            <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
                <Typography variant="h3" component="h1" sx={{ mb: 4, textAlign: 'center', fontWeight: 700 }}>
                    Your Repositories
                </Typography>

                {loading && <LoadingIndicator />}
                {error && <ErrorDisplay error={error} />}
                {!loading && !error && (
                    repoNames && repoNames.length > 0 ? (
                        <Grid container spacing={3}>
                            {repoNames.map((repoName, index) => (
                                <Grid item xs={12} sm={6} md={4} key={repoName}>
                                    <AnimatedListItem index={index}>
                                        <Card sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
                                            <CardActionArea component={RouterLink} to={`/repo/${repoName}/tree`} sx={{ flexGrow: 1 }}>
                                                <CardContent>
                                                    <Box sx={{ display: 'flex', alignItems: 'center', mb: 1.5 }}>
                                                        <FaCodeBranch size="1.5em" style={{ marginRight: '12px', color: '#6c757d' }} />
                                                        <Typography gutterBottom variant="h6" component="div" sx={{ fontWeight: '600', wordBreak: 'break-all' }}>
                                                            {repoName}
                                                        </Typography>
                                                    </Box>
                                                    <Typography variant="body2" color="text.secondary">
                                                         {/* Add real description later */}
                                                         Basic repository description placeholder.
                                                    </Typography>
                                                </CardContent>
                                            </CardActionArea>
                                        </Card>
                                    </AnimatedListItem>
                                </Grid>
                            ))}
                        </Grid>
                    ) : (
                        // Use Paper for better visual grouping of the empty message
                        <Paper sx={{ p: 3, textAlign: 'center', mt: 2 }}>
                             <Typography variant="body1" color="text.secondary">
                                 No repositories found. Use the `mygit init` command to create one.
                             </Typography>
                        </Paper>
                    )
                )}
            </Container>
        </PageTransition>
    );
}

export default HomePage;