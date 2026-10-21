import tkinter as tk
from vmm import Simulator, ALGORITHMS

# ---- theme ---------------------------------------------------------------
BG, CARD, LINE = "#150709", "#220c10", "#5a1b26"
TEXT, MUTE = "#ffe6e3", "#c98a8f"
RED, BAR, OK = "#ff3b4e", "#8f0b1f", "#3ddc84"
FONT = ("Courier New", 11)
FONT_B = ("Courier New", 11, "bold")
FONT_BIG = ("Courier New", 22, "bold")


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("VMM-SIM")
        self.configure(bg=BG)
        self.geometry("900x700")
        self.sim = None
        self.timer = None
        self.last_frame = -1
        self._build_topbar()
        self._build_body()
        self.reset()

    # ---- layout ----------------------------------------------------------
    def _build_topbar(self):
        bar = tk.Frame(self, bg=BAR, highlightthickness=0)
        bar.pack(fill="x")
        tk.Frame(self, bg=RED, height=3).pack(fill="x")   # red underline

        tk.Label(bar, text=">_ VMM-SIM", bg=BAR, fg=TEXT, font=("Courier New", 14, "bold")
                 ).pack(side="left", padx=14, pady=10)

        self.algo = tk.StringVar(value=ALGORITHMS[0])
        menu = tk.OptionMenu(bar, self.algo, *ALGORITHMS, command=lambda _: self.reset())
        menu.config(bg=BAR, fg=TEXT, font=FONT, activebackground=RED, highlightthickness=0, bd=1)
        menu["menu"].config(bg=CARD, fg=TEXT, font=FONT)
        menu.pack(side="left", padx=4)

        tk.Label(bar, text="frames", bg=BAR, fg=TEXT, font=FONT).pack(side="left", padx=(12, 4))
        self.nf = tk.Spinbox(bar, from_=1, to=8, width=3, font=FONT, command=self.reset)
        self.nf.delete(0, "end"); self.nf.insert(0, "3")
        self.nf.pack(side="left")

        for text, cmd in (("Reset", self.reset), ("Play", self.toggle_play), ("Step", self.step)):
            b = tk.Button(bar, text=text, command=cmd, bg=RED, fg="white", font=FONT_B,
                          relief="flat", activebackground="#ff6b7a", padx=12)
            b.pack(side="right", padx=4, pady=8)
            if text == "Play":
                self.play_btn = b

    def _card(self, title):
        wrap = tk.Frame(self, bg=RED)
        wrap.pack(fill="x", padx=16, pady=(12, 0))
        tk.Frame(wrap, bg=RED, width=4).pack(side="left", fill="y")
        card = tk.Frame(wrap, bg=CARD)
        card.pack(side="left", fill="both", expand=True)
        tk.Label(card, text=title, bg=CARD, fg=RED, font=FONT_B).pack(anchor="w", padx=10, pady=(8, 4))
        return card

    def _build_body(self):
        c = self._card("reference string")
        self.refs_entry = tk.Entry(c, bg=BG, fg=TEXT, insertbackground=TEXT, font=FONT, relief="flat")
        self.refs_entry.insert(0, "7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1")
        self.refs_entry.pack(fill="x", padx=10, pady=4)
        self.refs_entry.bind("<Return>", lambda e: self.reset())
        self.strip = tk.Frame(c, bg=CARD)
        self.strip.pack(anchor="w", padx=10, pady=(4, 10))

        c = self._card("physical memory")
        self.frames_box = tk.Frame(c, bg=CARD)
        self.frames_box.pack(anchor="w", padx=10, pady=(4, 10))

        c = self._card("stats")
        row = tk.Frame(c, bg=CARD)
        row.pack(anchor="w", padx=10, pady=(0, 10))
        self.stat = {}
        for key in ("hits", "faults", "hit ratio"):
            col = tk.Frame(row, bg=CARD)
            col.pack(side="left", padx=(0, 40))
            self.stat[key] = tk.Label(col, text="0", bg=CARD, fg=RED, font=FONT_BIG)
            self.stat[key].pack(anchor="w")
            tk.Label(col, text=key, bg=CARD, fg=MUTE, font=FONT).pack(anchor="w")

        c = self._card("log")
        self.log = tk.Text(c, height=8, bg=CARD, fg=TEXT, font=FONT, relief="flat", state="disabled")
        self.log.tag_config("hit", foreground=OK)
        self.log.tag_config("fault", foreground=RED)
        self.log.pack(fill="x", padx=10, pady=(0, 10))

    # ---- actions (UI -> logic) ---------------------------------------------
    def reset(self, *_):
        self.stop()
        try:
            refs = [int(x) for x in self.refs_entry.get().replace(",", " ").split()]
            n = max(1, min(8, int(self.nf.get())))
        except ValueError:
            return
        self.sim = Simulator(refs, n, self.algo.get())
        self.last_frame = -1
        self.log.config(state="normal"); self.log.delete("1.0", "end"); self.log.config(state="disabled")
        self.render()

    def step(self):
        self.stop()
        self._do_step()

    def _do_step(self):
        event = self.sim.step()
        if event is None:
            self.stop()
            return
        self.last_frame = -1 if event.hit else event.frame
        self.log.config(state="normal")
        self.log.insert("1.0", event.describe() + "\n", "hit" if event.hit else "fault")
        self.log.config(state="disabled")
        self.render()

    def toggle_play(self):
        if self.timer:
            self.stop()
            return
        if self.sim.done:
            self.reset()
        self.play_btn.config(text="Pause")
        self._tick()

    def _tick(self):
        self._do_step()
        if not self.sim.done and self.play_btn["text"] == "Pause":
            self.timer = self.after(600, self._tick)
        else:
            self.stop()

    def stop(self):
        if self.timer:
            self.after_cancel(self.timer)
            self.timer = None
        self.play_btn.config(text="Play")

    # ---- drawing -------------------------------------------------------------
    def render(self):
        s = self.sim
        for w in self.strip.winfo_children():
            w.destroy()
        for i, page in enumerate(s.refs):
            ev = s.history[i] if i < len(s.history) else None
            fg, bg = TEXT, BG
            if ev:
                fg, bg = (OK, BG) if ev.hit else ("white", RED)
            border = 2 if i == s.pos else 0
            tk.Label(self.strip, text=str(page), width=3, bg=bg, fg=fg, font=FONT_B,
                     highlightbackground=RED, highlightthickness=border
                     ).grid(row=i // 20, column=i % 20, padx=2, pady=2)

        for w in self.frames_box.winfo_children():
            w.destroy()
        for i, page in enumerate(s.frames):
            f = tk.Frame(self.frames_box, bg=BG, width=90, height=70,
                         highlightbackground=RED if i == self.last_frame else LINE,
                         highlightthickness=2)
            f.pack(side="left", padx=5)
            f.pack_propagate(False)
            tk.Label(f, text=f"frame {i}", bg=BG, fg=MUTE, font=("Courier New", 9)).pack(pady=(6, 0))
            tk.Label(f, text="-" if page is None else str(page), bg=BG, fg=TEXT, font=FONT_BIG).pack()

        self.stat["hits"].config(text=str(s.hits))
        self.stat["faults"].config(text=str(s.faults))
        self.stat["hit ratio"].config(text=f"{round(s.hit_ratio * 100)}%")


if __name__ == "__main__":
    App().mainloop()
