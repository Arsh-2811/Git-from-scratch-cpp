import React, { useState, useEffect } from 'react';
import { getAllRepositories } from '../../services/repositoryService'
import styles from './RepositoryList.module.css';

function RepositoryList() {
    const [repositories, setRepositories] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    useEffect(() => {
        const loadRepositories = async () => {
            setLoading(true);
            setError(null);

            try {
                const repoNames = await getAllRepositories();
                setRepositories(repoNames);
            } catch (err) {
                setError(err.message || 'An unexpected error occured.')
            } finally {
                setLoading(false);
            }
        };

        loadRepositories();
    }, []);

    const handleRepoClick = (repoName) => {
        console.log(`Clicked on repository: ${repoName}`);
        alert(`You clicked on ${repoName}. Navigation not yet implemented.`);
    };

    return (
        <div className={styles.container}>
            <h2 className={styles.title}>Repositories</h2>
            {loading && <p className={styles.loading}>Loading repositories...</p>}
            {error && <p className={styles.error}>Error: {error}</p>}
            {!loading && !error && (
                repositories.length > 0 ? (
                    <ul className={styles.list}>
                        {repositories.map((repoName) => (
                            <li
                                key={repoName}
                                className={styles.listItem}
                                onClick={() => handleRepoClick(repoName)}
                                title={`View ${repoName} details`}
                            >
                                <span className={styles.repoIcon}></span>
                                <span className={styles.repoName}>{repoName}</span>
                                <span className={styles.arrowIcon}>→</span>
                            </li>
                        ))}
                    </ul>
                ) : (
                    <p className={styles.emptyMessage}>No repositories found.</p>
                )
            )}
        </div>
    );
}

export default RepositoryList;