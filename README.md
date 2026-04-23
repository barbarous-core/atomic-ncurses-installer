# atomic-ncurses-installer

> An interactive ncurses TUI installer for **Fedora CoreOS** that generates a ready-to-use
> [Ignition](https://coreos.github.io/ignition/) (`.ign`) configuration file.

```
   BARBAROUS CORE                    Welcome                               v0.1
 ─────────────────────────────────────────────────────────────────────────────────

  ┌─────────────────────────────────────────────────────────────────────────────┐
  │  ██████╗  █████╗ ██████╗ ██████╗  █████╗ ██████╗  ██████╗ ██╗   ██╗███████╗ │
  │  ...                                                                         │
  │            Fedora CoreOS  ◆  Ignition File Generator                         │
  │  ─────────────────────────────────────────────────────────────────────────── │
  │           This installer will guide you through:                             │
  │             ◆  Locale, Timezone & User Configuration                         │
  │             ◆  Hostname & SSH Key Setup                                      │
  │             ◆  Barbarous OS Edition Selection                                │
  │             ◆  Package & Binary Customization                                │
  │             ◆  Target Disk Selection & Verification                          │
  │             ◆  Final Summary & System Installation                           │
  │                        [ ENTER  Begin ]                                      │
  └─────────────────────────────────────────────────────────────────────────────┘

 ─────────────────────────────────────────────────────────────────────────────────
  ENTER  Begin    Q  Quit
```

---

## What it does

Instead of hand-authoring Butane/Ignition YAML, this installer walks you through
every configuration decision and writes a validated `.ign` JSON file that you
pass directly to `coreos-installer`:

```bash
coreos-installer install /dev/sda --ignition-file ./barbarous.ign
```

The generated Ignition file handles:

| Step | Feature |
|------|---------|
| 1 | **Locale & User** — Keyboard layout, timezone, and user credentials |
| 2 | **Hostname & SSH** — Set system hostname and generate/inject SSH keys |
| 3 | **OS Edition** — Select Barbarous OS flavor (Core, Station, Studio, etc.) |
| 4 | **RPM & Binaries** — Select tool categories from a matrix of 100+ tools |
| 5 | **Disk Selection** — Pick target disk for installation with verification |
| 6 | **IGN Generation** — Final summary, dotfile toggle, and file writing |

---

## Requirements

| Dependency | Purpose | Website |
|-----------|---------|---------|
| `gcc` ≥ 11 | C11 compiler | [gcc.gnu.org](https://gcc.gnu.org/) |
| `ncurses-devel` | TUI library headers | [invisible-island.net/ncurses](https://invisible-island.net/ncurses/) |
| `pkg-config` | Library flag resolution | [freedesktop.org](https://www.freedesktop.org/wiki/Software/pkg-config/) |
| `openssl` | SHA-512 password hashing (runtime) | [openssl.org](https://www.openssl.org/) |
| `coreos-installer` | Actual CoreOS installation (runtime, on live ISO) | [github.com/coreos/coreos-installer](https://github.com/coreos/coreos-installer) |
| `chafa` | Generate ASCII art and animation (runtime) | [github.com/hpjansson/chafa](https://github.com/hpjansson/chafa) |

Install on Fedora:

```bash
sudo dnf install gcc ncurses-devel pkg-config openssl chafa
```

---

## Build

```bash
git clone https://github.com/barbarous-core/atomic-ncurses-installer.git
cd atomic-ncurses-installer
make
```

The binary is output as `./barbarous-install-tui`.

```bash
make run      # build + launch immediately
make clean    # remove objects and binary
```

---

## Usage

Run as root (required for disk access):

```bash
sudo ./barbarous-install-tui
```

Or, from a Barbarous live ISO where it ships as `barbarous-install`:

```bash
barbarous-install
```

### Keyboard navigation (all screens)

| Key | Action |
|-----|--------|
| `↑` / `↓` or `j` / `k` | Move selection |
| `ENTER` / `Space` | Confirm / Next |
| `Tab` | Cycle between fields / buttons |
| `Backspace` | Delete character in input fields |
| `←` / `ESC` | Previous screen |
| `Q` | Quit / cancel |

---

## Output

The installer writes a single Ignition JSON file (default: `./barbarous.ign`).
You can then install Fedora CoreOS with:

```bash
# On the live ISO
coreos-installer install /dev/sda \
    --ignition-file ./barbarous.ign

# Or embed it directly into the ISO
coreos-installer iso ignition embed \
    --ignition-file ./barbarous.ign \
    fedora-coreos.iso
```

---

## Project structure

```
atomic-ncurses-installer/
├── Makefile
└── src/
    ├── main.c              # ncurses lifecycle + screen router
    ├── installer.h         # shared installer_state_t struct
    ├── installer.c         # state initialisation / defaults
    ├── ui.h                # widget API declarations
    ├── ui.c                # header · footer · box · button · readline
    └── screens/
        ├── welcome.h/c     # ✅ Step 0 – splash / overview          [done]
        ├── locale.h/c      # ✅ Step 1 – locale, TZ & user          [done]
        ├── config.h/c      # ✅ Step 2 – hostname & SSH keys        [done]
        ├── edition.h/c     # ✅ Step 3 – OS edition selection       [done]
        ├── selection.h/c   # ✅ Step 4 – RPM & binary matrix        [done]
        ├── disk.h/c        # ✅ Step 5 – target disk selection      [done]
        └── generate.h/c    # ✅ Step 6 – summary & IGN generation   [done]
```

---

## Architecture

```
main.c
  └── installer_init()        set sane defaults into installer_state_t
  └── screen router loop
        case 0 → screen_welcome()   ─┐
        case 1 → screen_locale()     │  each screen:
        case 2 → screen_config()     │   • reads/writes installer_state_t
        case 3 → screen_edition()    │   • returns NAV_NEXT / NAV_PREV / NAV_QUIT
        case 4 → screen_selection()  │
        case 5 → screen_disk()      ─┘
        case 6 → screen_generate()  → writes barbarous.ign
```

Every screen is self-contained and only communicates through the shared
`installer_state_t` struct — easy to add, remove, or reorder steps.

---

## Relationship to Barbarous Core

This installer is part of the [Barbarous](https://github.com/barbarous-core) project —
a curated Fedora CoreOS-based distribution. The generated `.ign` file is equivalent
to the hand-authored `barbarous-core.bu` Butane config, but produced interactively
without requiring the user to know Ignition syntax.

---

## License

MIT
