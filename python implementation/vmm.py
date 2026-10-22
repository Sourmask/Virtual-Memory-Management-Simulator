"""Virtual memory logic. No UI code in here, so it can be tested or reused anywhere."""
from dataclasses import dataclass
from typing import Optional

ALGORITHMS = ("FIFO", "LRU", "OPT")


@dataclass
class Event:
    time: int                 # 1-based step number
    page: int                 # page that was requested
    hit: bool                 # True = already in memory, False = page fault
    frame: int                # frame that now holds the page
    evicted: Optional[int]    # page that was kicked out (None if hit or free frame used)

    def describe(self) -> str:
        if self.hit:
            return f"HIT    page {self.page} is in frame {self.frame}"
        if self.evicted is None:
            return f"FAULT  page {self.page} loaded into free frame {self.frame}"
        return f"FAULT  page {self.page} replaces page {self.evicted} in frame {self.frame}"


class Simulator:
    def __init__(self, refs, num_frames=3, algo="FIFO"):
        if algo not in ALGORITHMS:
            raise ValueError(f"algo must be one of {ALGORITHMS}")
        self.refs = list(refs)
        self.algo = algo
        self.frames = [None] * num_frames   # page stored in each frame
        self.loaded_at = [0] * num_frames   # used by FIFO
        self.used_at = [0] * num_frames     # used by LRU
        self.pos = 0                        # next index in refs
        self.hits = 0
        self.faults = 0
        self.history = []                   # list of Event

    # ---- state helpers -------------------------------------------------
    @property
    def done(self) -> bool:
        return self.pos >= len(self.refs)

    @property
    def hit_ratio(self) -> float:
        total = self.hits + self.faults
        return self.hits / total if total else 0.0

    # ---- replacement policy -------------------------------------------
    def _pick_victim(self) -> int:
        if self.algo == "FIFO":
            return self.loaded_at.index(min(self.loaded_at))
        if self.algo == "LRU":
            return self.used_at.index(min(self.used_at))
        # OPT: evict the page whose next use is furthest away
        victim, farthest = 0, -1
        for i, page in enumerate(self.frames):
            try:
                nxt = self.refs.index(page, self.pos + 1)
            except ValueError:
                nxt = float("inf")
            if nxt > farthest:
                victim, farthest = i, nxt
        return victim

    # ---- one memory access --------------------------------------------
    def step(self) -> Optional[Event]:
        if self.done:
            return None
        page, t = self.refs[self.pos], self.pos + 1
        evicted = None

        if page in self.frames:                       # HIT
            frame = self.frames.index(page)
            self.hits += 1
            self.used_at[frame] = t
            hit = True
        else:                                         # PAGE FAULT
            self.faults += 1
            hit = False
            if None in self.frames:
                frame = self.frames.index(None)
            else:
                frame = self._pick_victim()
                evicted = self.frames[frame]
            self.frames[frame] = page
            self.loaded_at[frame] = t
            self.used_at[frame] = t

        self.pos += 1
        event = Event(t, page, hit, frame, evicted)
        self.history.append(event)
        return event


if __name__ == "__main__":
    # quick check: classic textbook string with 3 frames -> FIFO 15 faults, LRU 12, OPT 9
    s = "7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1".split()
    for a in ALGORITHMS:
        sim = Simulator(map(int, s), 3, a)
        while sim.step():
            pass
        print(a, "faults:", sim.faults, "hits:", sim.hits)
