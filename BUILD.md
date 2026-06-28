# Building the Haystack C++ Core

How to compile the C++ search engine, run its tests, and launch the binaries.

---

## 1. Prerequisites

All of these are installed via [Homebrew](https://brew.sh) on macOS.

| Tool | Purpose | Install |
|------|---------|---------|
| **CMake** ≥ 3.20 | Build system | `brew install cmake` |
| **clang++** (C++20) | Compiler | Ships with Xcode Command Line Tools: `xcode-select --install` |
| **Drogon** | C++ HTTP framework (used by `searchd`) | `brew install drogon` |
| **Tesseract** | OCR engine (Phase 2.5 ingestion) | `brew install tesseract` |
| **Leptonica** | Image processing (OCR dependency) | `brew install leptonica` |
| **Poppler** | PDF text extraction | `brew install poppler` |

> Catch2 (the test framework) is **not** installed manually — CMake fetches it
> automatically via `FetchContent` the first time you configure.

### Verify your setup
```bash
cmake --version          # expect >= 3.20
clang++ --version
brew list | grep -E 'drogon|tesseract|leptonica|poppler'
```

---

## 2. Build location

| | Path |
|---|------|
| **Run commands from** (project root, holds `CMakeLists.txt`) | `/Users/vilastadoori/_Haystack_proj` |
| **Build output goes to** (compiled binaries land here) | `build/` |

You run the build *from the root* — you do **not** `cd` into `build/` to compile.

---

## 3. Compile (Mac Terminal)

From the project root:

```bash
# 1. Configure — generates build files into ./build (safe to re-run)
cmake -S . -B build

# 2. Compile everything (-j uses all CPU cores)
cmake --build build -j
```

### Build a single target (faster)
```bash
cmake --build build --target haystack -j        # main CLI only
cmake --build build --target searchd -j         # HTTP server only
cmake --build build --target unit_tests -j      # unit tests only
cmake --build build --target runtime_tests -j   # runtime/integration tests only
```

### Clean rebuild from scratch
```bash
rm -rf build && cmake -S . -B build && cmake --build build -j
```

---

## 4. Build artifacts

After a successful build, these appear in `build/`:

| Binary | Description |
|--------|-------------|
| `haystack` | Main CLI executable |
| `searchd` | Search daemon — Drogon HTTP server on port **8900** |
| `unit_tests` | Catch2 unit-test suite |
| `runtime_tests` | Runtime/integration tests (incl. Phase 2.5 PDF/OCR ingestion) |
| `libsearch_core.a` | Static lib: tokenizer, inverted index, query parser, search service, snippet |
| `libingestion_phase25.a` | Static lib: Phase 2.5 file scanner, txt/pdf processors, OCR policy, aggregator |

---

## 5. Running the tests

### Via CTest (runs everything registered in CMake)
```bash
cd build && ctest --output-on-failure
```

### Run a test binary directly (more granular)
```bash
./build/unit_tests --verbosity high
./build/runtime_tests --verbosity high
```

> Runtime tests spawn subprocesses (e.g. `searchd`) with timeouts and run
> serially, so they take longer than the unit tests.

---

## 6. Running the binaries

```bash
./build/haystack          # main CLI
./build/searchd           # starts the HTTP search server on port 8900
```

---

## 7. Building from VS Code (CMake Tools extension)

1. Install the **CMake Tools** extension (publisher: Microsoft).
2. `Cmd+Shift+P` → **CMake: Configure** (select a Clang kit if prompted).
3. `Cmd+Shift+P` → **CMake: Build**, or press **F7**, or use the **Build**
   button in the bottom status bar.
4. Use the status-bar **target selector** to build a single target, and the
   ▶️ / 🐞 buttons to run or debug.

---

## 8. Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Could not find a package configuration file provided by "Drogon"` | `brew install drogon`, then re-run `cmake -S . -B build` |
| Stale/weird build errors after editing `CMakeLists.txt` | Clean rebuild: `rm -rf build && cmake -S . -B build && cmake --build build -j` |
| Catch2 download fails on first configure | Check network; FetchContent clones Catch2 `v3.5.4` from GitHub |
| OCR/PDF tests fail to find tools | Ensure `tesseract`, `leptonica`, `poppler` are installed via brew |

---

**C++ standard:** C++20 (project) / C++17 (`search_core`, `ingestion_phase25` libs)
**Build system:** CMake + Make generator
**Test framework:** Catch2 v3.5.4 (auto-fetched)
