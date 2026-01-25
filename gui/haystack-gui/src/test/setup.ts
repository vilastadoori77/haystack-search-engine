import { expect, afterEach } from 'vitest';
import { cleanup } from '@testing-library/react';

import * as matchers from '@testing-library/jest-dom/matchers';

// extends Vitest's expect method with methods from jest-dom
expect.extend(matchers);

// automatically unmount and cleanup DOM after the test is finished.
afterEach(() => {
    cleanup();
});
