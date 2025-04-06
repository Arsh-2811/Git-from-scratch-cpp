// src/styles/theme.js
import { createTheme } from '@mui/material/styles';

const theme = createTheme({
    palette: {
        primary: {
            main: '#1976d2', // Blue
        },
        secondary: {
            main: '#dc004e', // Pink/Red
        },
        background: {
            default: '#f4f6f8', // Light grey background
            paper: '#ffffff',
        },
    },
    typography: {
        fontFamily: '"Roboto", "Helvetica", "Arial", sans-serif',
        h1: { fontSize: '2.5rem', fontWeight: 500 },
        h2: { fontSize: '2rem', fontWeight: 500 },
        h3: { fontSize: '1.75rem', fontWeight: 500 },
        h4: { fontSize: '1.5rem', fontWeight: 500 },
        h5: { fontSize: '1.25rem', fontWeight: 500 },
        h6: { fontSize: '1rem', fontWeight: 500 },
    },
    components: {
        MuiAppBar: {
            styleOverrides: {
                root: {
                    backgroundColor: '#24292e', // GitHub dark header
                }
            }
        },
        MuiLink: { // Consistent link styling
            styleOverrides: {
                 root: {
                      textDecoration: 'none',
                      color: '#1976d2', // Use primary color for links
                      '&:hover': {
                          textDecoration: 'underline',
                      }
                 }
            }
        }
    }
});

export default theme;