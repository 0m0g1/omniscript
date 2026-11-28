# Game Boy (DMG) emulator in OmniScript: plan and handoff

Written so a new chat can continue without the old one. Attach the files listed under "Files" when you start it.

## Status (read this first)

- **A playable application exists:** `gameboy.os` loads a ROM, runs it in a window at 3x with sound, battery saves and an MBC3 clock. **2048 (homebrew) runs.** Other real games are untried.
- **Core verified by running (before the mapper/APU work):** `test_gb.os`; all 11 Blargg `cpu_instrs` ROMs, `instr_timing`, combined `cpu_instrs.gb` (13 of 13 in `gb_run.os` MODE 0); dmg-acid2 with 0 differing pixels (`gb_acid2.os`).
- **Written but not yet verified by running:** MBC2, MBC3 (+RTC), MBC5, the whole APU (sound quality and accuracy), the RTC save format, the mute key. Re-run `test_gb.os`, `gb_run.os` and `gb_acid2.os` once to confirm the CPU/PPU did not regress.
- **Audio output verified:** the OpenAL streaming test (`gbsound_al_test.os`) produced sound. The miniaudio variant (`gbsound.os`) was written but not confirmed.
- **Not done:** CGB (color) games, rumble, ROMs over 2 MB, accurate APU edge cases, pixel FIFO, save states, key remapping, settings file, in-app ROM browser.

## Files

All live in `docs/examples/gb/` in the user's project (`C:\dev\omniscript\omniscript`) unless stated. Engine: `.\bin\Release-windows-x86_64\Osengine.exe`, run from the project root in PowerShell.

| File | What it is |
|---|---|
| `gb.os` | The core: CPU, bus, PPU, timers, interrupts, mappers, RTC, APU. `GB.GameBoy` struct, fixed global storage, about 1800 lines. |
| `gameboy.os` | **The application.** Window, input, palettes, pause/reset/fast-forward/mute, battery + clock saves, sound output. `ROM_PATH` variable at the top; also accepts the ROM as `argv[1]` (unverified when run through `Osengine.exe`). |
| `gbsound_al.os` | PCM output on OpenAL (`PcmOut`: `open(rate)`, `push`, `queued`, `space`, `close`). **The one `gameboy.os` imports.** |
| `gbsound.os` | Alternative PCM output on miniaudio's ring buffer (`ma_pcm_rb`), needs the existing `Audio.Engine`. Optional; unverified. |
| `standard/1/c/openal.os` | OpenAL Soft binding (`soft_oal.dll` under `dependencies/openal-soft-1.24.3-bin/bin/Win64/`). Two functions were added for streaming: `alSourceQueueBuffers`, `alSourceUnqueueBuffers`. |
| `gbsound_al_test.os`, `gbsound_test.os` | One-off 3-second square-wave checks for the two audio backends. |
| `test_gb.os` | Smoke test, no ROM file needed. Run after any change to `gb.os`. |
| `gb_run.os` | Headless runner. `MODE` 0 Blargg batch (13 ROMs), 1 Gameboy Doctor trace. |
| `gb_acid2.os` | Runs dmg-acid2, prints differing pixels and a tile diff map. |
| `gb_play.os` | The older window demo (superseded by `gameboy.os`). |
| `OMNISCRIPT_SYNTAX_GUIDE.md` | Language rules (v7). Section 0 must be followed. |
| `GAMEBOY_PLAN.md` | This file. |
| `roms/` | `gb-test-roms/`, `dmg-acid2.gb`, `dmg-acid2-reference-dmg.png`, `games/` (downloaded homebrew). |

Test ROM URLs (from the project root):
```
git clone https://github.com/retrio/gb-test-roms docs/examples/gb/roms/gb-test-roms
Invoke-WebRequest https://github.com/mattcurrie/dmg-acid2/releases/download/v1.0/dmg-acid2.gb -OutFile docs/examples/gb/roms/dmg-acid2.gb
Invoke-WebRequest https://raw.githubusercontent.com/mattcurrie/dmg-acid2/master/img/reference-dmg.png -OutFile docs/examples/gb/roms/dmg-acid2-reference-dmg.png
git clone https://github.com/robert/gameboy-doctor tools/gameboy-doctor
```
Gameboy Doctor: set `MODE = 1` in `gb_run.os`, run it, then `python tools/gameboy-doctor/gameboy-doctor gb_trace.log cpu_instrs <TRACE_ROM>`.

Free games (Homebrew Hub database, `gbdev/database`; the `2048gb` entry is the one confirmed to work):
```
New-Item -ItemType Directory -Force docs/examples/gb/roms/games | Out-Null
Invoke-WebRequest https://raw.githubusercontent.com/gbdev/database/master/entries/2048gb/2048.gb -OutFile docs/examples/gb/roms/games/2048.gb

git clone --filter=blob:none --no-checkout --depth 1 https://github.com/gbdev/database docs/examples/gb/roms/hhdb
cd docs/examples/gb/roms/hhdb
git ls-tree --name-only HEAD entries/ | Select-String "tobu|2048|libbet"
git sparse-checkout init --cone
git sparse-checkout set entries/2048gb entries/<slug>
git checkout master
```
Entries with `"platform": "GBC"` are color-only and will not run. Tobu Tobu Girl is on itch.io (manual download).

## Application (`gameboy.os`)

Keys: arrows = d-pad, Z = A, X = B, Enter = Start, Backspace = Select, P pause, R reset (keeps save RAM), Tab (hold) fast-forward, C colour palette (4), M mute, Esc quit.

- Emulation is paced by the wall clock (frame = 70224 clocks, up to 4 frames of catch-up per display frame, `dt` capped at 0.1 s).
- Saves: `<rom path>.sav` next to the ROM. Written every 5 s when RAM changed, on reset, and on exit. Layout: cart RAM (`ramBytes` bytes), then for MBC3 carts with a clock a 16-byte record (live S M H DL DH, marker 0xA5 at byte 5, unix time at bytes 8 to 11, little endian). On load the clock catches up on the real time that passed. Only carts with a battery get a `.sav`.
- Sound: after each emulated frame the core's samples are pushed to `PcmOut` (dropped when muted, paused or fast-forwarding). Buffering: 8 OpenAL buffers of 1024 frames; playback starts once 2 are queued and restarts after an underrun. Emulation timing follows the wall clock, not the audio device, so the queue can slowly over- or underfill (extra samples are dropped, underruns are silence).
- Packaging (from the file header, unverified): `Osengine.exe docs/examples/gb/gameboy.os --make -o gameboy.exe`, then drop a `.gb` onto the exe or run `gameboy.exe game.gb`.

## Core contract (as implemented in `gb.os`)

Import: `module GB = import "gb.os";`. `let gb: GB.GameBoy;` on the stack works. Storage is fixed global arrays (ROM 2 MB cap, cart RAM 32 KB, VRAM, WRAM, OAM, HRAM, IO, framebuffer, APU registers, audio buffer, serial log). Nothing is heap-allocated. Registers are plain `int` fields kept masked.

| Member | Meaning |
|---|---|
| `reset()` | Post-boot DMG state (AF=01B0 BC=0013 DE=00D8 HL=014D SP=FFFE PC=0100), APU on with post-boot registers. Clears all RAM **including cart RAM**; does not touch the MBC3 clock. |
| `loadRom(data: u8*, len: int) => bool` | Copies into fixed storage, sets mapper/RAM/battery/clock flags and the title, calls `reset()`. Accepted cart types: 00, 01 to 03 (MBC1), 05, 06 (MBC2), 08, 09 (no mapper + RAM), 0F to 13 (MBC3), 19 to 1E (MBC5; rumble ignored). Rejected: other types, ROM size code above 6 (2 MB), RAM code above 3 (32 KB), ROMs larger than declared. |
| `step() => int` | One instruction, interrupt dispatch (20 clocks) or HALT idle step (4). Returns clocks. |
| `runFrame(budget: int) => int` | Steps until at least `budget` clocks are spent (70224 per frame). |
| `willExecute() => bool` | True if the next `step()` fetches an instruction. |
| `setKey(idx: int, down: bool)` | 0 Right, 1 Left, 2 Up, 3 Down, 4 A, 5 B, 6 Select, 7 Start. |
| `framebuffer() => u8*`, `width()`, `height()` | 160x144, one byte per pixel, shade 0 to 3. |
| `title() => char*` | Printable header title (up to 16 characters, null-terminated). |
| `battery`, `rtc`, `ramBytes` | Public fields set by `loadRom`: battery-backed, has MBC3 clock, cart RAM size (MBC2 = 512). |
| `ramPtr() => u8*` | The cart RAM array (`ramBytes` valid bytes). |
| `rtcDirty` | Public bool, set when the game writes a clock register. The app clears it after saving. |
| `rtcAdvance(secs: int)` | Runs the MBC3 clock by real seconds (no-op without a clock or when halted). |
| `rtcExport(out: u8*, now: int)`, `rtcImport(inp: u8*, now: int)` | 16-byte clock record described above. `now` is unix seconds. |
| `audioCount() => int`, `audioPtr() => i16*`, `audioClear()` | Samples produced since the last clear: interleaved signed 16-bit stereo at 44100 Hz, up to 4096 frames (about 738 per emulated frame). |
| `soundActive() => bool` | True when the APU is powered (NR52 bit 7). |
| `serialText() => char*`, `serialCount() => int` | Serial log (null-terminated, 4 KB). The core never prints. |
| `trace(out: Str.String*)`, `setDoctor(on: bool)` | Gameboy Doctor line / FF44 reads 0x90. |
| Public fields | `ly`, `frameDone`, `pc`, `sp`, `a`..`l`, `f`, `ime`, etc. `frameDone` is set at VBlank and never cleared by the core. |

## What is built, by stage

| Stage | Done | Exit test | Result |
|---|---|---|---|
| 1 CPU | base and CB opcodes via x/y/z decode, flags, DAA, cycle counts, serial capture, trace | individual `cpu_instrs` | pass |
| 2 PPU | scanline renderer: BG, window with line counter, 8x8/8x16 sprites, flips, priority, 10-per-line, OAM DMA, STAT/LYC | dmg-acid2 | 0 differing pixels |
| 3 Timers | DIV/TIMA/TMA/TAC on one 16-bit counter, falling-edge detection | `02-interrupts`, `instr_timing` | pass |
| 4 Interrupts | IE/IF/IME, vectors, delayed EI, RETI, HALT wake-up, HALT bug, combined STAT line | `02-interrupts` | pass |
| 5 MBC1 | RAM enable, 5-bit ROM bank, 2-bit upper bank / RAM bank, banking mode | combined `cpu_instrs.gb` | pass |
| 6 Mappers | MBC2 (512x4-bit RAM), MBC3 (7-bit ROM bank, RAM bank 0 to 3, clock registers 08 to 0C with latch), MBC5 (9-bit ROM bank, 4-bit RAM bank), no-mapper + RAM | **not run** | written |
| 7 APU | 4 channels, frame sequencer, sweep, envelopes, length, panning, NR50/NR51/NR52, wave RAM, DAC model, box-filtered 44.1 kHz output with DC-blocking high-pass | played through 2048 only (audio quality unconfirmed) | written |
| 8 Application | window, input, saves, sound output | 2048 runs | works |

PPU details: 154 lines of 456 dots, advanced 4 dots at a time. Mode 2 is 80 dots, mode 3 a fixed 172, a line is drawn at the mode 3 to 0 transition. LCDC bit 0 off gives a white background and no window. Sprite priority: lower X wins, then lower OAM index. No pixel FIFO.

APU details: registers FF10 to FF26 live in the global `AR[]` (indexed by IO offset); wave RAM stays in `IORAM`. Ticked every 4 clocks from `advance`. Frame sequencer every 8192 clocks (length at steps 0/2/4/6, sweep at 2/6, envelope at 7). Channel timers count in clocks (square: (2048 - f) * 4, wave: (2048 - f) * 2, noise: divisor << shift). Sample emitted every 4194304 / 44100 clocks as the average of the mixed ticks, then a high-pass (factor 0.996) and a fixed gain of 50. Powering off clears registers FF10 to FF25; writes are ignored while off.

## Known simplifications (refine only if a game or test needs it)

- OAM DMA is instant. VRAM and OAM are always readable and writable.
- Mode 3 is fixed at 172 dots. No mid-line register effects. The first line after enabling the LCD starts in mode 2.
- No STAT mode-2 interrupt on line 144. No LY=153 early-wrap quirk.
- WY trigger is latched when a line is rendered.
- PPU, timers and APU advance after each instruction in 4-clock steps, not per memory access.
- TIMA reload has no delay. The serial interrupt is never raised.
- Invalid opcodes act as 1-cycle NOPs. STOP is a NOP that skips one byte.
- APU: no length-counter and trigger quirks (extra length clock, sweep negate-disable, zombie envelope writes), channel timer low bits not preserved on trigger, wave RAM access while playing returns the stored byte, no per-model filtering.
- MBC1 multicart, MBC6/7, HuC1/3, rumble and cameras are not supported. ROMs are capped at 2 MB and RAM at 32 KB.
- DMG only: Game Boy Color-only ROMs will not run.

## Engine APIs known

- **Canvas loop (confirmed):** `let app: C.Canvas; app.init(); let win = app.createWindowEx(title, w, h, C.API_GL);` then `while (app.frame()) { ... win.context2d() ...; app.present(); }`, then `app.quit()`. Check `win.isOpen`; `win.close()` closes.
- **Input:** `win.keyDown(k)`, `win.keyPressed(k)`. GLFW codes used: Esc 256, Enter 257, Tab 258, Backspace 259, Right 262, Left 263, Down 264, Up 265, Z 90, X 88, P 80, R 82, C 67, M 77.
- **Pen2D:** `clear`, `fillRect(x, y, w, hh: float, c: u32)` (0xRRGGBBAA), `text`, `textScaled`, `createImage(px: u8*, w, hh) => u32`, `image(tex, x, y, w, hh)`, `freeImage(id)`. Images are batched: free the previous frame's image at the start of the next frame (`gameboy.os` does).
- **Time:** `Time.monotonicNs() => u64`, `Time.unixSeconds()` (used with `as int`), `Time.sleep(ms: u64)`.
- **IO:** `IO.File` with `open(path, IO.MODE_READ|MODE_WRITE)`, `size()`, `readBytes`, `writeBytes`, `close()` (sequential reads work for the .sav layout).
- **FFI:** `extern "path/to.dll" { fn name(args) => ret; }` works for any DLL (`miniaudio.dll` is built from the full `miniaudio.h`, so every `ma_*` function is available; OpenAL Soft binding in `standard/1/c/openal.os`).
- **Audio.os (std):** file playback only (`Engine`, `Sound`). PCM streaming is in the project's own `gbsound_al.os`, not in the standard library.

## Next work (suggested order)

1. **Try real games** and fix what breaks: Tobu Tobu Girl, other Homebrew Hub games, ideally one MBC1, one MBC3 (clock) and one MBC5 title. Report the game name and what is wrong (black screen, garbled graphics, wrong sound, crash). Then regress with `test_gb.os`, `gb_run.os`, `gb_acid2.os`.
2. **Accuracy upgrades only if a game needs them:** PPU mode-3 length / FIFO, OAM DMA timing, STAT quirks, APU trigger and length quirks (Blargg `dmg_sound`).
3. **Quality of life:** settings file (key map, scale, palette), in-app ROM browser or file dialog, window resize/fullscreen (`--doc-show standard/1/Canvas.os`), save states, audio/video sync from the audio queue depth.
4. **Coverage:** Game Boy Color (second VRAM bank, palettes, double speed), MBC1 multicart, ROMs above 2 MB.

## Working conventions for the AI

- The user wants **a complete, working product delivered in one go, not step-by-step stages or extra test scripts.** Build everything needed for the feature, wire it into `gameboy.os`, and only ask when something truly cannot be decided.
- When files are uploaded, edit them directly and present the edited files; put each file's path as a comment at the top; no zip files for code deliverables.
- Follow `OMNISCRIPT_SYNTAX_GUIDE.md` section 0: use only documented syntax and APIs, never guess a module function, ask for the exact `--doc-show` or `--doc-search` output when unsure. Plain C APIs through `extern` DLL bindings are fine.
- If valid-looking code fails to compile, do not contort it. Give a minimal repro and say it looks like a compiler gap. The author fixes the compiler.
- Found by running: `char*` `==` is not a content compare. `Console.print` takes only `char*`; numbers need `Console.println`. No `return` inside loop bodies (use a flag). `|=` style compound assignment is not documented; write `x = x | y`. Load `u8` array elements into `int` locals before arithmetic.
- Worked in this project: stack `GB.GameBoy` struct using global `u8[N]` / `int[N]` / `i16[N]` arrays; `RBUF as void*` for `readBytes`; passing a global array where a pointer is expected; `Console.println(&String)`; `extern` DLL bindings.
- The user is on Windows with PowerShell. Paths are relative to the project root, not the script. Settings are variables at the top of a file; script arguments are unverified.
- After every change to `gb.os`, run `test_gb.os` first, then `gb_run.os` and `gb_acid2.os`.

## Starting the new chat

Attach: `OMNISCRIPT_SYNTAX_GUIDE.md`, `GAMEBOY_PLAN.md`, `gb.os`, `gameboy.os`, `gbsound_al.os`, `test_gb.os`, `gb_run.os`, `gb_acid2.os`. Suggested first message: "Continue the Game Boy emulator from GAMEBOY_PLAN.md. Here is what I tried and what went wrong: ..."
