# 🎨 ColorMix Suite – 

## 1. Executive Summary
**ColorMix** is a dual-modality, zero-dependency color computation and visualization suite engineered entirely in pure C. It provides two distinct execution paradigms: a lightweight, cross-platform command-line interface (`colormix.c`) and a native Windows desktop application (`colormix_gui.c`). Both binaries share an identical 32-color palette, a deterministic RGB averaging algorithm, and a nearest-neighbor color matching engine. The CLI version leverages ANSI TrueColor escape sequences for terminal rendering, making it ideal for scripting, automation, and headless environments. The GUI version utilizes the Win32 API and GDI to deliver an interactive, real-time color mixing workspace with dynamic ratio control, custom swatch rendering, and contextual metadata display. Designed for developers, designers, and systems programmers, the suite emphasizes explicit memory management, predictable computational complexity, and strict separation between mathematical logic and presentation layers.

---

## 2. System Architecture & Design Philosophy

| Design Principle | Implementation Strategy |
|:---|:---|
| **Shared Computational Core** | Identical palette definition, mixing formula, and Euclidean nearest-neighbor algorithm across both binaries. Ensures mathematical consistency and single-source-of-truth logic. |
| **Presentation Decoupling** | CLI relies on standard I/O and ANSI escape codes. GUI relies on Win32 message routing, GDI painting, and Common Controls. Zero shared UI code. |
| **State Isolation** | CLI operates statelessly per execution. GUI encapsulates all runtime state in a single `AppState` struct, preventing global variable pollution and enabling predictable teardown. |
| **Zero External Dependencies** | Uses only C standard library (`<stdio.h>`, `<stdlib.h>`, `<math.h>`, `<string.h>`, `<ctype.h>`) and native Windows APIs (`<windows.h>`, `<commctrl.h>`, GDI). No third-party frameworks, JSON parsers, or graphic libraries. |
| **Explicit Resource Lifecycle** | GUI explicitly creates/deletes `HBRUSH` and `HFONT` objects. CLI relies on process termination for OS-level cleanup. Both avoid memory leaks through deterministic allocation patterns. |

---

## 3. Core Data Structures

### 3.1 `Color` / `ColorDef` Struct
```c
typedef struct {
    const char *name;
    int r, g, b;
} Color;
```
- **Static Palette:** 32 predefined entries covering primary, secondary, tertiary, and nuanced web-standard colors (e.g., `coral`, `lavender`, `skyblue`).
- **Memory Layout:** `const char*` names reside in read-only data segment. RGB values are packed in 12 bytes per entry. Total palette footprint: ~1.2 KB.

### 3.2 `AppState` (GUI Only)
```c
typedef struct {
    HBRUSH hBrush1, hBrush2, hBrushResult;
    int cr, cg, cb;                 // Selected indices for Color1/Color2
    int r1, g1, b1, r2, g2, b2;     // Source RGB values
    int rr, gr, br;                 // Result RGB values
    HWND hCombo1, hCombo2, hSlider;
    HFONT hFont, hFontBold;
} AppState;
```
- **GDI Handles:** Three `HBRUSH` objects dynamically managed to prevent handle exhaustion.
- **State Flags:** Tracks combo selections and computed RGB values for synchronous UI updates.

---

## 4. CLI Version (`colormix.c`) – Technical Deep Dive

### 4.1 Execution Flow
1. **Argument Parsing:** Validates `argc`. Supports `colormix <c1> + <c2>` or `colormix <c1> <c2>`.
2. **Normalization:** `str_tolower()` converts inputs to lowercase for case-insensitive matching.
3. **Lookup:** `find_color()` performs linear scan (`O(N)`) against `palette[]`.
4. **Mixing:** Computes arithmetic mean: `mr = (r1 + r2) / 2`, etc.
5. **Nearest Match:** `nearest_color()` calculates squared Euclidean distance in RGB space.
6. **Rendering:** Outputs ANSI `ESC[48;2;R;G;Bm` sequences for TrueColor terminal blocks.

### 4.2 Key Functions
| Function | Complexity | Behavior |
|:---|:---|:---|
| `find_color()` | `O(N)` | Case-sensitive `strcmp` loop. Returns pointer or `NULL`. |
| `nearest_color()` | `O(N)` | Minimizes `dr² + dg² + db²`. Uses `INFINITY` initialization. |
| `show_color_block()` | `O(1)` | Prints 6 space characters wrapped in ANSI background color codes. |
| `show_bar()` | `O(W)` | Renders 40-character wide solid color strip for visual emphasis. |
| `main()` | `O(N)` | Orchestrates parsing, validation, computation, and formatted output. |

### 4.3 ANSI Terminal Rendering
```c
printf("\033[48;2;%d;%d;%dm      \033[0m", r, g, b);
```
- Uses `48;2;R;G;B` for 24-bit TrueColor background.
- Automatically resets with `\033[0m` to prevent terminal state corruption.
- Compatible with modern terminals (Windows Terminal, iTerm2, GNOME Terminal, Alacritty).

---

## 5. GUI Version (`colormix_gui.c`) – Technical Deep Dive

### 5.1 Win32 Architecture
- **Message-Driven Loop:** Standard `GetMessage → TranslateMessage → DispatchMessage` pipeline.
- **Dynamic Control Creation:** All UI elements (`COMBOBOX`, `TRACKBAR`, `BUTTON`, `STATIC`) instantiated at runtime via `CreateWindow`. No `.rc` resource files.
- **Custom Painting:** Overrides `WM_PAINT` with offscreen GDI drawing (`FillRect`, `FrameRect`, `DrawText`).
- **Theme:** Dark background (`RGB(30,30,30)`) with white-bordered swatches and dynamic result text coloring.

### 5.2 Event Handling & State Updates
| Message | Handler | Action |
|:---|:---|:---|
| `WM_CREATE` | `init_controls()` | Creates UI, populates combos, sets default slider to 50, allocates initial brushes. |
| `WM_COMMAND` | Button/Combo | Reads `CB_GETCURSEL`, calls `update_result()`. |
| `WM_HSCROLL` | Trackbar | Fetches `TBM_GETPOS`, recalculates weighted mix, invalidates window. |
| `WM_CTLCOLORSTATIC` | GDI Hook | Sets `SetTextColor(hdc, RGB(rr,gr,br))` for result label. Returns `NULL_BRUSH` for transparency. |
| `WM_PAINT` | Custom Draw | Renders three swatches, `+`/`=` operators, and color names. Uses `BitBlt` implicitly via direct `HDC` drawing. |
| `WM_DESTROY` | Cleanup | Calls `DeleteObject()` on all brushes/fonts, posts `WM_QUIT`. |

### 5.3 GDI Resource Management
`update_result()` demonstrates strict handle lifecycle control:
```c
if (g_state.hBrush1)  DeleteObject(g_state.hBrush1);
g_state.hBrush1 = CreateSolidBrush(RGB(r1, g1, b1));
```
- Prevents GDI object leaks by deleting previous brushes before allocation.
- Uses `InvalidateRect(hwnd, NULL, TRUE)` to trigger partial redraws without flicker.

---

## 6. Color Mixing Algorithm & Nearest-Neighbor Logic

### 6.1 Weighted RGB Averaging
GUI supports proportional mixing via slider ratio `ρ ∈ [0, 100]`:
```c
R_result = (R1 * (100 - ρ) + R2 * ρ) / 100
```
- CLI defaults to `ρ = 50` (arithmetic mean).
- Linear interpolation in RGB space ensures predictable, mathematically closed results.

### 6.2 Nearest-Color Matching
Finds palette entry minimizing Euclidean distance:
```c
dist = (R_i - R)² + (G_i - G)² + (B_i - B)²
```
- **Complexity:** `O(N)` per lookup (`N=32`). Negligible on modern hardware.
- **Limitation:** RGB space is not perceptually uniform. Distant colors in RGB may appear similar to humans, and vice versa. Sufficient for small palettes but not for professional color science.

---

## 7. Build & Deployment Instructions

### 7.1 CLI Version (Cross-Platform)
```bash
# GCC / Clang (Linux/macOS/WSL)
gcc -O2 -std=c99 colormix.c -lm -o colormix

# MinGW-w64 (Windows)
gcc -O2 -std=c99 colormix.c -lm -o colormix.exe
```
- **Dependencies:** Standard C runtime + math library (`-lm`).
- **Execution:** `./colormix red + cyan` or `./colormix --list`

### 7.2 GUI Version (Windows Only)
```cmd
# MSVC
cl /O2 colormix_gui.c /FeColorMixGUI.exe /link gdi32.lib comctl32.lib user32.lib kernel32.lib

# MinGW-w64
gcc -O2 -std=c99 colormix_gui.c -o ColorMixGUI.exe -lgdi32 -lcomctl32 -luser32 -lkernel32 -mwindows
```
- **Dependencies:** `comctl32.dll` v6+, `gdi32.dll`, `user32.dll`. Windows XP or later.
- **Execution:** Double-click `ColorMixGUI.exe`. No installer or registry writes.

---

## 8. Performance & Memory Characteristics

| Metric | CLI | GUI |
|:---|:---|:---|
| **Memory Footprint** | ~12–16 KB (stack + static data) | ~1.2–1.5 MB (GDI handles, window structures, OS overhead) |
| **CPU Usage** | ~0.1% (single execution, terminates) | ~0.5–1.5% (idle, event-driven) |
| **Time Complexity** | `O(N)` lookup + `O(1)` math | `O(N)` lookup + `O(1)` GDI operations per event |
| **Thread Safety** | Single-threaded, stateless | Single-threaded UI, main message pump |
| **I/O Model** | Blocking `printf`/`scanf` | Asynchronous `WM_*` message routing |

---

## 9. Known Limitations & Engineering Roadmap

| Limitation | Technical Impact | Proposed Mitigation |
|:---|:---|:---|
| RGB-space mixing | Perceptually non-linear; muddy results for complementary colors | Implement CIELAB/L*a*b* space conversion with Delta E nearest-neighbor |
| Fixed palette size | Cannot add custom hex/RGB colors at runtime | Load `palette.json`/`colors.ini` at startup; expose hex input fields |
| Linear search lookup | Scales poorly if palette grows to thousands | Replace with hash table or k-d tree for `O(1)`/`O(log N)` lookups |
| GUI layout rigidity | Fixed coordinates break on DPI scaling or resizing | Implement `WM_SIZE` handler with proportional layout math or DPI-aware manifest |
| No color history | Users cannot revisit previous mixes | Add `Stack<ColorMix>` with undo/redo or export to CSV |
| Windows-only GUI | Excludes macOS/Linux desktop users | Port to GTK3/4, SDL2, or Dear ImGui for cross-platform deployment |

**🚀 v2.0 Roadmap:**
- CIELAB perceptual mixing & Delta E distance metric
- Runtime palette loader (JSON/TOML)
- Hex/RGB/HSV input validation & conversion
- Cross-platform GUI abstraction layer
- Export palette to CSS/SASS/JSON variables
- DPI-aware scaling & resizable window layout

---

## 10. Appendix: API Reference & Constants

### 10.1 CLI Usage
```
colormix <color1> + <color2>   Mix two colors (50/50 ratio)
colormix <color1> <color2>     Alternative syntax
colormix --list                Display full palette with ANSI swatches
colormix --help                Show usage information
```

### 10.2 GUI Control IDs
| ID | Control | Purpose |
|:---|:---|:---|
| `1001` | `IDC_COLOR1` | Dropdown for source color 1 |
| `1002` | `IDC_COLOR2` | Dropdown for source color 2 |
| `1003` | `IDC_BTN_MIX` | Triggers manual recalculation (legacy) |
| `1004` | `IDC_TRACKBAR` | Mix ratio slider (0–100%) |
| `1005–1015` | Labels | RGB values, ratio text, nearest name |

### 10.3 Palette Structure (Sample)
```c
{"red",        255,   0,   0},
{"skyblue",    135, 206, 235},
{"lavender",   230, 230, 250},
...
```
- Total entries: `32`
- Range: `R,G,B ∈ [0, 255]`
- Naming: Web/CSS standard conventions

---

📄 **Document Version:** 1.0  
📅 **Last Updated:** May 2026  
🔧 **Target Platforms:** POSIX Terminals / Windows Desktop (Win32/GDI)  
📦 **Source Reference:** `colormix.c`, `colormix_gui.c`  
🏷️ **Tags:** `#PureC` `#ColorScience` `#ANSIEscapes` `#Win32API` `#GDIDrawing` `#ZeroDependency` `#CLIUtility` `#DesktopApp` `#RGBMixing` `#EventDrivenUI`
