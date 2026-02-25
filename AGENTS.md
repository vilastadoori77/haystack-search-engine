# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

Haystack is a C++ search platform with a React GUI. It has two main services:
- **searchd** (C++ HTTP backend on port 8900) — indexes and serves search queries via `/search` and `/health` endpoints, built on the Drogon web framework
- **haystack-gui** (React/Vite frontend on port 5173) — proxies API calls to searchd

### Building the C++ backend

The project **must** be built with GCC (not Clang). The default compiler on this VM is Clang, which lacks filesystem support for Drogon. Use:

```bash
cd /workspace && mkdir -p build && cd build
CC=gcc-14 CXX=g++-14 cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

This produces three binaries in `build/`: `searchd`, `unit_tests`, `runtime_tests`.

### Running tests

- **C++ unit tests:** `cd /workspace/build && ./unit_tests --verbosity high` (209 assertions, 85 test cases)
- **C++ runtime tests:** `cd /workspace/build && ./runtime_tests --verbosity high` (spawns searchd subprocesses)
- **GUI tests:** `cd /workspace/gui/haystack-gui && npx vitest run` (180 tests)
- **GUI lint:** `cd /workspace/gui/haystack-gui && npm run lint`
- **GUI build:** `cd /workspace/gui/haystack-gui && npm run build` (runs `tsc -b && vite build`)

### Running the application

1. Create an index from documents (JSON or text directory):
   ```bash
   ./build/searchd --index --docs data/docs.json --out idx
   ```
2. Start the backend:
   ```bash
   ./build/searchd --serve --in idx --port 8900
   ```
3. Start the GUI dev server:
   ```bash
   cd /workspace/gui/haystack-gui && npx vite --host 0.0.0.0 --port 5173
   ```

### Key gotchas

- The `libstdc++.so` symlink may be missing on fresh VMs. If CMake fails with `cannot find -lstdc++`, run: `sudo ln -sf /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/libstdc++.so`
- Drogon must be installed from source (not available via apt). It is installed at `/usr/local/lib/cmake/Drogon/`.
- The Vite dev server proxies `/search` and `/health` to `localhost:8900`, so searchd must be running for the GUI to function fully.
- PDF ingestion requires optional `poppler-utils` and `tesseract` packages — not needed for basic JSON/text document loading.
