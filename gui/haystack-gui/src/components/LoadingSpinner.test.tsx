
import { render, screen } from '@testing-library/react';
import LoadingSpinner from './LoadingSpinner';


describe('LoadingSpinner Component', () => {
    it('renders the loading spinner', () => {
        render(<LoadingSpinner />);
        expect(screen.getByRole('status')).toBeInTheDocument();
    });

    it('displays "Searching..." text', () => {
        render(<LoadingSpinner />);
        expect(screen.getByText('Searching...')).toBeInTheDocument();
    });

    it('has aria-label for accessibility', () => {
        render(<LoadingSpinner />);
        expect(screen.getByRole('status')).toHaveAttribute('aria-label', 'Loading');
    });
});
