import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/search': 'http://localhost:8900',
      '/health': 'http://localhost:8900',
    },
  },
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: './src/test/setup.ts',
    // Reduce CPU usage - run tests sequentially instead of in parallel
    maxWorkers: 1,
    minWorkers: 1,
    // Reduce test timeout for faster failure detection
    testTimeout: 10000,
  },
})
