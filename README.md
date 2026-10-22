# Virtual Memory Management Simulator

VMM-SIM demonstrates how operating systems manage a limited number of physical-memory frames. Its browser interface uses the C simulation engine compiled to WebAssembly, so the page replacement decisions are made by C while the browser displays each step, frame snapshot, log entry, and statistic.

## Run the browser simulator

### Requirements

- Emscripten SDK (`emcc`) to compile the C engine to WebAssembly.
- Python 3, Node.js, or another simple local HTTP server. Browsers generally block loading WebAssembly from a `file://` URL.

### Build and serve

From the project directory, build the browser module:

```powershell
.\build-wasm.bat
```

Run this from an Emscripten-enabled terminal where `emcc` is on `PATH`. The build creates `vmm.js` and `vmm.wasm` beside `index.html`.

Start a local server in the same directory:

```powershell
python -m http.server 8000
```

Then open <http://localhost:8000/> in a browser. Keep the server running while using the page. The browser UI is not a second JavaScript implementation of the replacement policies: it calls the C engine in WebAssembly for initialization, each memory access, frame state, and statistics. JavaScript handles input validation, animation, presentation, and explanatory text.

Choose FIFO, LRU, or OPT, set 1–8 frames, enter non-negative integer page references separated by spaces or commas, then use **Step** or **Play**. **Reset** applies the current settings. Play speed is adjustable. The default reference string is `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1`.

If the browser reports that the WebAssembly files are missing, run the build step and reload the page. If `emcc` is not recognized, activate/install the Emscripten SDK first.

## Python desktop implementation

The original Tkinter desktop version is kept separately under [`python implementation/`](./python%20implementation/). Run it from the project root with:

```powershell
python ".\python implementation\app.py"
```

[`app.py`](./python%20implementation/app.py) imports the Python simulation engine from [`vmm.py`](./python%20implementation/vmm.py) in the same folder. Python 3.7 or later and a working Tkinter installation are required. The Python implementation is independent of the C/WebAssembly browser simulator.

## C command-line simulator

The same C engine is also available as a native command-line program. It requires GCC or Clang:

```powershell
gcc -std=c99 -Wall -Wextra -O2 main.c vmm.c -o vmm.exe
.\vmm.exe
```

Select the policy, frame count, and references with command-line options:

```powershell
.\vmm.exe --algorithm LRU --frames 3 --refs "7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1"
```

Supported algorithms are `FIFO`, `LRU`, and `OPT`. Frame counts must be 1–8. References are non-negative integers separated by spaces or commas; quote the reference string so it is passed as one argument. Use `--help` to display usage. The program prints each access and frame state, followed by totals and hit ratio.

For the built-in 20-reference example with three frames, the expected fault counts are FIFO: 15, LRU: 12, and OPT: 9.

## How the simulation works

A reference string is an ordered sequence of page numbers. Each physical frame stores one page or is empty. For each reference:

1. If the requested page is already resident, it is a **hit**.
2. Otherwise it is a **page fault**. The page is loaded into a free frame if one is available.
3. If all frames are occupied, a replacement policy chooses a victim and the requested page takes its frame.

### Replacement algorithms

- **FIFO (First-In, First-Out):** Evicts the page that has been in its frame the longest. A hit does not change the page's load time.
- **LRU (Least Recently Used):** Evicts the page whose most recent access is oldest. A hit updates its last-used time.
- **OPT (Optimal):** Evicts the resident page whose next use is farthest in the remaining reference string. A page never used again is an optimal victim. OPT uses future knowledge for comparison; an operating system cannot know future references in advance.

When candidates tie, the lowest-index frame is selected.

### Statistics

- **Accesses:** Number of references processed.
- **Hits:** References already present in a frame.
- **Faults:** References that required loading a page.
- **Hit ratio:** `hits / accesses`, or zero before any accesses.

Every access is either a hit or a fault, so `accesses = hits + faults`.

## C engine API

[`vmm.h`](./vmm.h) declares the public API and data types; [`vmm.c`](./vmm.c) implements the simulator. The browser adapter in [`vmm_web.c`](./vmm_web.c) exposes the engine to the WebAssembly interface. The main API operations are:

- `vmm_init(...)` initializes simulator state and frame bookkeeping.
- `vmm_step(...)` processes one reference and fills a `VMMEvent`; it returns `false` when the sequence is complete.
- `vmm_done(...)` checks whether all references have been processed.
- `vmm_hit_ratio(...)` returns the ratio from 0.0 to 1.0.
- `vmm_destroy(...)` releases simulator allocations.

The engine borrows the reference array; it must remain valid while the simulator uses it. Each event contains the 1-based step, requested page, hit/fault result, frame index, and optional evicted page.

## Project files

| File | Purpose |
| --- | --- |
| [`index.html`](./index.html) | Browser UI, controls, snapshots, explanations, and statistics. |
| [`vmm.c`](./vmm.c) | Shared C page-replacement engine. |
| [`vmm.h`](./vmm.h) | C simulator API and data types. |
| [`vmm_web.c`](./vmm_web.c) | C-to-JavaScript/WebAssembly interface for the browser. |
| [`build-wasm.bat`](./build-wasm.bat) | Emscripten build command for `vmm.js` and `vmm.wasm`. |
| [`main.c`](./main.c) | Native C command-line interface and input parser. |
| [`python implementation/app.py`](./python%20implementation/app.py) | Tkinter desktop interface implemented in Python. |
| [`python implementation/vmm.py`](./python%20implementation/vmm.py) | Python simulation engine used by the desktop app. |

The browser uses the C engine compiled to WebAssembly. The native CLI also uses the C engine. The Python desktop implementation is retained separately in `python implementation/`.
