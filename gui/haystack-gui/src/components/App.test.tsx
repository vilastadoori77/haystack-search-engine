import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { render, screen, waitFor, fireEvent } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import axios from 'axios';
import MockAdapter from 'axios-mock-adapter';
import App from '../App';

describe('App', () => {
  let mockAxios: MockAdapter;

  beforeEach(() => {
    mockAxios = new MockAdapter(axios);
    vi.useFakeTimers();
  });

  afterEach(() => {
    mockAxios.restore();
    vi.useRealTimers();
    vi.clearAllMocks();
  });

  // TC-064: User can search and see results end-to-end
  it('TC-064: user can search and see results end-to-end', async () => {
    // Given: App component with mocked successful API response
    const mockResults = [
      { docId: 1, score: 0.95, snippet: 'First result' },
      { docId: 2, score: 0.87, snippet: 'Second result' },
    ];
    mockAxios.onGet(/\/search\?q=.*/).reply(200, {
      query: 'migration',
      results: mockResults,
    });
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: User types query and clicks search
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'migration');
    const searchButton = screen.getByRole('button', { name: 'Search' });
    await user.click(searchButton);

    // Then: Results are displayed and metrics are shown
    await waitFor(() => {
      expect(screen.getByText(/first result/i)).toBeInTheDocument();
      expect(screen.getByText(/second result/i)).toBeInTheDocument();
    });

    // And: Metrics panel shows query and result count
    const metricsPanel = screen.getByRole('region', { name: 'Search metrics' });
    expect(metricsPanel).toHaveTextContent('migration');
    expect(screen.getByText(/2\s*result/i)).toBeInTheDocument();
  });

  // TC-065: Error message displays when server unavailable
  it('TC-065: error message displays when server unavailable', async () => {
    // Given: App component with mocked network error
    mockAxios.onGet('/search').networkError();
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: User searches
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'test');
    const searchButton = screen.getByRole('button', { name: 'Search' });
    await user.click(searchButton);

    // Then: Error message is displayed in ResultsList
    await waitFor(() => {
      const errorMessage = screen.getByText(/network error|server unavailable|error/i);
      expect(errorMessage).toBeInTheDocument();
    });
  });

  // TC-066: Example query auto-executes search
  it('TC-066: example query auto-executes search', async () => {
    // Given: App component with mocked successful API response
    const mockResults = [{ docId: 1, score: 0.95, snippet: 'Result' }];
    mockAxios.onGet(/\/search\?q=.*/).reply(200, {
      query: 'migration',
      results: mockResults,
    });
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: User clicks example query button
    const exampleButton = screen.getByRole('button', { name: /search for:\s*migration$/i });
    await user.click(exampleButton);

    // Then: Query is set and search is triggered automatically
    await waitFor(() => {
      expect(screen.getByDisplayValue('migration')).toBeInTheDocument();
      expect(screen.getByText('Result')).toBeInTheDocument();
    });
  });

  // TC-067: Health indicator updates automatically
  it('TC-067: health indicator updates automatically', async () => {
    // Given: App component with mocked health endpoint
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: Component mounts
    // Then: Health check is performed
    await waitFor(() => {
      expect(mockAxios.history.get).toContainEqual(
        expect.objectContaining({ url: '/health' })
      );
    }, { timeout: 2000 });

    // When: Time advances by 5 seconds
    await new Promise(resolve => setTimeout(resolve, 5100));

    // Then: Health check is performed again
    await waitFor(() => {
      const healthCalls = mockAxios.history.get.filter((call) => call.url === '/health');
      expect(healthCalls.length).toBeGreaterThan(1);
    }, { timeout: 2000 });
  });

  // TC-068: Limit slider affects search results
  it('TC-068: limit slider affects search results', async () => {
    // Given: App component with mocked API
    const mockResults = Array.from({ length: 25 }, (_, i) => ({
      docId: i + 1,
      score: 0.9 - i * 0.01,
      snippet: `Result ${i + 1}`,
    }));
    mockAxios.onGet(/\/search\?q=.*/).reply((config) => {
      const url = config.url || '';
      const urlParams = new URLSearchParams(url.split('?')[1] || '');
      const k = parseInt(urlParams.get('k') || '10', 10);
      return [200, { query: 'test', results: mockResults.slice(0, k) }];
    });
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: User sets limit to 25 and searches
    const slider = screen.getByRole('slider') as HTMLInputElement;
    fireEvent.change(slider, { target: { value: '25' } });
    await waitFor(() => {
      expect(screen.getByText('Limit: 25')).toBeInTheDocument();
    });
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'test');
    const searchButton = screen.getByRole('button', { name: 'Search' });
    await user.click(searchButton);

    // Then: API is called with k=25
    await waitFor(() => {
      const searchCall = mockAxios.history.get.find((call) => call.url?.startsWith('/search?'));
      expect(searchCall).toBeDefined();
      if (searchCall?.url) {
        const urlParams = new URLSearchParams(searchCall.url.split('?')[1]);
        expect(urlParams.get('k')).toBe('25');
      }
    });
  });

  // TC-069: Clear button resets state
  it('TC-069: clear button resets state', async () => {
    // Given: App component with search results
    const mockResults = [{ docId: 1, score: 0.95, snippet: 'Result' }];
    mockAxios.onGet(/\/search\?q=.*/).reply(200, {
      query: 'test',
      results: mockResults,
    });
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // Perform search
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'test');
    const searchButton = screen.getByRole('button', { name: 'Search' });
    await user.click(searchButton);

    await waitFor(() => {
      expect(screen.getByText('Result')).toBeInTheDocument();
    });

    // When: Clear button is clicked
    const clearButton = screen.getByRole('button', { name: /clear/i });
    await user.click(clearButton);

    // Then: Query is empty, results are cleared, metrics are cleared
    expect(screen.getByPlaceholderText('Enter your search query...')).toHaveValue('');
    expect(screen.queryByText('Result')).not.toBeInTheDocument();
    expect(screen.queryByText(/query:\s*test/i)).not.toBeInTheDocument();
  });

  // TC-070: Loading state shows during API call
  it('TC-070: loading state shows during API call', async () => {
    // Given: App component with delayed API response
    mockAxios.onGet(/\/search\?q=.*/).reply(() => {
      return new Promise((resolve) => {
        setTimeout(() => {
          resolve([200, { query: 'test', results: [] }]);
        }, 100);
      });
    });
    mockAxios.onGet('/health').reply(200, { status: 'healthy' });

    const user = userEvent.setup({ delay: null });
    vi.useRealTimers(); // Use real timers for this test
    render(<App />);

    // When: User searches
    const input = screen.getByPlaceholderText('Enter your search query...');
    await user.type(input, 'test');
    const searchButton = screen.getByRole('button', { name: 'Search' });
    await user.click(searchButton);

    // Then: Loading spinner is visible during request
    expect(screen.getByRole('status', { name: 'Loading' })).toBeInTheDocument();

    // When: Response arrives
    await waitFor(() => {
      expect(screen.queryByRole('status', { name: 'Loading' })).not.toBeInTheDocument();
    });
  });
});
