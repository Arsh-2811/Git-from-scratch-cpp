// src/components/common/AnimatedListItem.js
import React from 'react';
import { motion } from 'framer-motion';

const itemVariants = {
    hidden: { opacity: 0, y: 20 },
    visible: (i) => ({ // Accept custom index for staggering
        opacity: 1,
        y: 0,
        transition: {
            delay: i * 0.05, // Stagger delay
            duration: 0.3,
            ease: "easeOut",
        },
    }),
};

// Use this to wrap individual items in a list (e.g., <ListItem>, <Card>)
function AnimatedListItem({ children, index = 0, className = "" }) { // Added className prop
    return (
        <motion.div
            custom={index} // Pass index to variants
            initial="hidden"
            animate="visible"
            variants={itemVariants}
            layout // Animate layout changes if list order changes
            className={className} // Pass className down
        >
            {children}
        </motion.div>
    );
}

export default AnimatedListItem;