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
  │             ◆  Disk & partition selection                                    │
  │             ◆  User account & password                                       │
  │             ◆  Keyboard layout & timezone                                    │
  │             ◆  RPM package selection                                         │
  │             ◆  Binary & dotfile injection                                    │
  │             ◆  Ignition file generation                                      │
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
| 1 | **Disk selection** — enumerate block devices via `lsblk`, pick install target |
| 2 | **User account** — username + salted SHA-512 password hash (`openssl passwd`) |
| 3 | **Locale** — keyboard layout (XKB) + timezone (zoneinfo) |
| 4 | **RPM packages** — select packages to layer with `rpm-ostree` from local ISO |
| 5 | **Binary injection** — copy binaries from ISO into `/usr/local/bin` |
| 6 | **Dotfile injection** — deploy dotfiles to the user's home directory |
| 7 | **IGN generation** — write `barbarous.ign` (or custom path) to disk |

---

## Requirements

| Dependency | Purpose |
|-----------|---------|
| `gcc` ≥ 11 | C11 compiler |
| `ncurses-devel` | TUI library headers |
| `pkg-config` | Library flag resolution |
| `openssl` | SHA-512 password hashing (runtime) |
| `coreos-installer` | Actual CoreOS installation (runtime, on live ISO) |

Install on Fedora:

```bash
sudo dnf install gcc ncurses-devel pkg-config openssl
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
    ├── ui.c                # header · footer · box · button · readline · msgbox
    └── screens/
        ├── welcome.h/c     # ✅ Step 0 – splash / overview          [done]
        ├── disk.h/c        # ✅ Step 1 – block device selection     [done]
        ├── user.h/c        # ✅ Step 2 – user + password            [done]
        ├── locale.h/c      # ✅ Step 3 – keyboard + timezone        [done]
        ├── packages.h/c    # ✅ Step 4 – RPM package picker         [done]
        ├── binaries.h/c    # ✅ Step 5 – binary injection           [done]
        ├── dotfiles.h/c    # ✅ Step 6 – dotfile injection          [done]
        └── generate.h/c    # ✅ Step 7 – IGN file writer + summary  [done]
```

---

## Architecture

```
main.c
  └── installer_init()        set sane defaults into installer_state_t
  └── screen router loop
        case 0 → screen_welcome()   ─┐
        case 1 → screen_disk()       │  each screen:
        case 2 → screen_user()       │   • reads/writes installer_state_t
        case 3 → screen_locale()     │   • returns NAV_NEXT / NAV_PREV / NAV_QUIT
        case 4 → screen_packages()   │
        case 5 → screen_binaries()  ─┘
        case 6 → screen_dotfiles()
        case 7 → screen_generate()  → writes barbarous.ign
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
