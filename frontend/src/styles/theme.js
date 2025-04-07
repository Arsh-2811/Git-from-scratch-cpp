// src/styles/theme.js
import { createTheme, alpha } from '@mui/material/styles';

const primaryColor = '#2d6cdf'; // A slightly richer blue
const secondaryColor = '#e91e63'; // A vibrant pink
const backgroundColor = '#f8f9fa'; // Slightly off-white
const paperColor = '#ffffff';
const textColor = '#212529';
const subduedTextColor = '#6c757d';

const theme = createTheme({
    palette: {
        primary: {
            main: primaryColor,
        },
        secondary: {
            main: secondaryColor,
        },
        background: {
            default: backgroundColor,
            paper: paperColor,
        },
        text: {
            primary: textColor,
            secondary: subduedTextColor,
        },
        action: {
            hover: alpha(primaryColor, 0.08), // Subtle hover for actions
        },
        divider: alpha(textColor, 0.12),
        info: { // Define info color for branch refs
             main: '#0d6efd', // Bootstrap blue
        },
        warning: { // For tags
             main: '#ffc107', // Bootstrap warning
        },
        success: { // For HEAD ref
             main: '#198754', // Bootstrap success
        }
    },
    typography: {
        fontFamily: '"Inter", "Roboto", "Helvetica", "Arial", sans-serif', // Modern font
        h1: { fontSize: '2.8rem', fontWeight: 700, letterSpacing: '-0.5px' },
        h2: { fontSize: '2.4rem', fontWeight: 700 },
        h3: { fontSize: '2rem', fontWeight: 600 },
        h4: { fontSize: '1.6rem', fontWeight: 600 },
        h5: { fontSize: '1.3rem', fontWeight: 600 },
        h6: { fontSize: '1.1rem', fontWeight: 600 },
        button: {
            textTransform: 'none', // Less shouting buttons
            fontWeight: 600,
        },
    },
    shape: {
        borderRadius: 8, // Slightly more rounded corners
    },
    components: {
        MuiAppBar: {
            styleOverrides: {
                root: {
                    backgroundColor: '#343a40', // Darker header
                    boxShadow: 'none', // Flatter look, rely on border/content separation
                    // borderBottom: `1px solid ${alpha(paperColor, 0.2)}`, // Can look heavy
                },
            },
        },
        MuiPaper: {
             defaultProps: {
                 elevation: 0, // Default to flat paper, use borders/backgrounds
                 variant: "outlined", // Use outlined variant by default
             },
             styleOverrides: {
                 root: {
                     // border: `1px solid ${alpha(textColor, 0.1)}`, // Handled by variant="outlined"
                     backgroundColor: paperColor, // Ensure paper background is white
                 }
             }
        },
        MuiCard: { // Style cards used for repo list
             defaultProps: {
                  variant: "outlined",
             },
            styleOverrides: {
                root: {
                    transition: 'transform 0.3s ease-in-out, box-shadow 0.3s ease-in-out',
                    '&:hover': {
                        transform: 'translateY(-4px)',
                        boxShadow: `0 4px 20px ${alpha(primaryColor, 0.1)}`,
                        borderColor: primaryColor, // Highlight border on hover
                    },
                },
            },
        },
        MuiListItemButton: {
             styleOverrides: {
                 root: {
                      borderRadius: 6, // Rounded list items
                      '&:hover': {
                          backgroundColor: alpha(primaryColor, 0.06), // Consistent hover
                      }
                 }
             }
        },
        MuiTooltip: {
            styleOverrides: {
                tooltip: {
                    backgroundColor: alpha('#111111', 0.9), // Darker tooltip
                    fontSize: '0.8rem',
                    borderRadius: 4,
                    padding: '6px 10px',
                },
                arrow: {
                    color: alpha('#111111', 0.9),
                }
            }
        },
        MuiTab: {
             styleOverrides: {
                 root: {
                      textTransform: 'none',
                      fontWeight: 500,
                      paddingLeft: '20px', // More padding
                      paddingRight: '20px',
                 }
             }
        },
        MuiChip: {
            styleOverrides: {
                root: {
                    fontWeight: 500,
                }
            }
        }
    },
});

export default theme;