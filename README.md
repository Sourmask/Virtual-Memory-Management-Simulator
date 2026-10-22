# Virtual Memory Management Simulator

Virtual Memory Management Simulator (VMM-SIM) is an educational application for exploring how operating systems manage a limited number of physical-memory frames. It replays a page-reference string one access at a time, shows hits and page faults, and demonstrates how FIFO, LRU, and OPT choose a page to replace when memory is full.

The repository contains two interfaces:

- A standalone browser application in [`index.html`](./index.html), with its simulation logic implemented in JavaScript.
- A desktop application in [`app.py`](./app.py), which uses the reusable Python simulation engine in [`vmm.py`](./vmm.py).

The browser and desktop applications work independently; the browser version does not call the Python module.

## Features

- Choose a page-replacement algorithm: FIFO, LRU, or OPT.
- Choose between 1 and 8 physical-memory frames.
- Enter a page-reference string as non-negative integers separated by spaces or commas.
- Advance one memory access at a time or play the simulation automatically.
- Inspect frame contents after each access and see which page was loaded or evicted.
- Track access, hit, and fault counts and the hit ratio.
- Review a chronological log of simulation events.

The browser interface also lets you adjust playback speed and gives a written explanation for each frame replacement. The desktop interface provides the same core simulation controls and statistics in a compact Tkinter window.

## Run the application

### Browser application

Open `index.html` in a modern web browser. No server, package installation, or build step is required.

1. Enter a reference string, or use the provided example.
2. Select an algorithm and frame count.
3. Select **Step** to process one reference, or **Play** to run continuously.
4. Select **Reset** to start again with the current inputs.

The browser interface runs entirely in the page and does not send reference strings to a server.

### Python desktop application

Python 3.7 or later is required for the standard-library `dataclasses` module. A working Tkinter installation is also required; some Linux distributions provide Tkinter as a separate system package.

From the project directory, run:

```bash
python app.py
```

On systems where the Python 3 command is named `python3`, use:

```bash
python3 app.py
```

Choose an algorithm and frame count from the top bar. Use **Step** to process one access, **Play** to run the remaining references automatically, **Pause** to stop playback, or **Reset** to restart. The default reference string is prefilled; edit it and press Enter to apply it.

The reusable Python engine can also be imported independently:

```python
from vmm import ALGORITHMS, Simulator

references = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
simulator = Simulator(references, num_frames=3, algo="LRU")

while (event := simulator.step()) is not None:
    print(event.describe())

print("Hits:", simulator.hits)
print("Faults:", simulator.faults)
print("Hit ratio:", simulator.hit_ratio)
```

For Python versions before 3.8, replace the assignment expression in this example with a conventional loop:

```python
while True:
    event = simulator.step()
    if event is None:
        break
    print(event.describe())
```

`algo` must be one of the names in `ALGORITHMS`: `"FIFO"`, `"LRU"`, or `"OPT"`. `step()` returns an `Event` for the next reference, or `None` when the reference string has been processed. Each event includes its 1-based step number, requested page, whether it was a hit, the frame involved, and any evicted page.

## How the simulation works

### Virtual pages and physical frames

A reference string is an ordered sequence of page numbers requested by a process. The simulator models physical memory as a fixed number of frames. A frame contains one page or is empty.

For each reference:

1. If the requested page is already in a frame, the access is a **hit**. The page stays in memory.
2. Otherwise, the access is a **page fault**. The page is loaded into an empty frame if one is available.
3. If all frames are occupied, the selected replacement policy chooses a victim page. The victim is evicted and the requested page takes its frame.

The Python `Simulator` maintains the frames, the next reference position, per-frame load and use times, hit and fault totals, and an event history. The browser version maintains equivalent state in JavaScript and additionally records a snapshot of the frames for each displayed step.

### Replacement algorithms

- **FIFO (First-In, First-Out):** Evicts the page that has been in its current frame the longest. A page's load time is updated when it is loaded into a frame, not when it is hit.
- **LRU (Least Recently Used):** Evicts the page whose most recent access is oldest. A page's last-used time is updated on both a hit and a load.
- **OPT (Optimal):** Evicts the resident page whose next reference is farthest in the future. If a page is never referenced again, it is an optimal victim. This policy looks ahead through the complete reference string, so it is useful for comparison and teaching; a real operating system cannot know future references in advance.

If there is a tie when choosing a victim, the first matching frame is selected.

### Statistics

- **Accesses:** References processed so far.
- **Hits:** References found in an existing frame.
- **Faults:** References that required loading a page.
- **Hit ratio:** `hits / accesses`. It is displayed as a percentage in the interfaces. Before any access, the ratio is zero.

Every processed reference is either a hit or a fault, so `accesses = hits + faults`.

## Project files

| File | Purpose |
| --- | --- |
| [`index.html`](./index.html) | Self-contained browser UI, styling, JavaScript simulation, playback, frame snapshots, explanations, and statistics. |
| [`app.py`](./app.py) | Tkinter desktop interface. Uses `Simulator` and `ALGORITHMS` from `vmm.py`. |
| [`vmm.py`](./vmm.py) | Reusable Python simulation engine, event model, and command-line demonstration. |

Run the Python engine's built-in example with:

```bash
python vmm.py
```

It processes a textbook reference string with three frames and prints hit and fault totals for all three algorithms. The expected fault totals for that example are FIFO: 15, LRU: 12, and OPT: 9.

## Input and behavior notes

- The interfaces start with the same example reference string: `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1`.
- The browser parser accepts non-negative integer pages separated by whitespace or commas and ignores tokens that are not valid non-negative integers.
- The desktop app parses whitespace-separated integers; commas are not treated as separators there.
- The desktop UI constrains its frame selector to 1–8. The browser clamps its frame count to that range.
- The Python engine handles an empty reference sequence with no accesses and a zero hit ratio. In the browser UI, clearing the input currently parses as a single reference to page `0`.

## Testing

No separate automated test suite is included in the repository. The `python vmm.py` command is a small built-in smoke check of the Python engine and the expected textbook results.
