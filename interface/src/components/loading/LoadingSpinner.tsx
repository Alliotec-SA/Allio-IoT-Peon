import { FC } from 'react';

import { CircularProgress, Box, Typography, Theme } from '@mui/material';

interface LoadingSpinnerProps {
  height?: number | string;
  message?: string;
  size?: number | string;
  variant?: 'h4' | 'h5' | 'h6' | 'subtitle1' | 'subtitle2';
}

const LoadingSpinner: FC<LoadingSpinnerProps> = ({ height = '100%', message = "Loading&hellip;", size = 100, variant = 'h4' }) => (
  <Box display="flex" alignItems="center" justifyContent="center" flexDirection="column" padding={2} height={height}>
    <CircularProgress
      sx={(theme: Theme) => ({
        margin: theme.spacing(4),
        color: theme.palette.text.secondary
      })}
      size={size}
    />
    <Typography variant={variant} color="textSecondary">
      {message}
    </Typography>
  </Box>
);

export default LoadingSpinner;
