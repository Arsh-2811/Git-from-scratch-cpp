// src/components/common/PageTransition.js
import React from 'react';
import { motion } from 'framer-motion';

const pageVariants = {
    initial: {
        opacity: 0,
        y: 10, // Subtle slide up
    },
    in: {
        opacity: 1,
        y: 0,
    },
    out: {
        opacity: 0,
        y: -10, // Subtle slide up and out
    },
};

const pageTransition = {
    type: 'tween',
    ease: 'anticipate', // A bit more dynamic easing
    duration: 0.4,
};

function PageTransition({ children }) {
    return (
        <motion.div
            initial="initial"
            animate="in"
            exit="out"
            variants={pageVariants}
            transition={pageTransition}
        >
            {children}
        </motion.div>
    );
}

export default PageTransition;