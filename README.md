# music-drums

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![GitHub all releases](https://img.shields.io/github/downloads/rgglez/music-drums/total)
![GitHub issues](https://img.shields.io/github/issues/rgglez/music-drums)
![GitHub commit activity](https://img.shields.io/github/commit-activity/y/rgglez/music-drums)
[![GitHub release](https://img.shields.io/github/release/rgglez/music-drums.svg)](https://github.com/rgglez/music-drums/releases/)
![GitHub stars](https://img.shields.io/github/stars/rgglez/music-drums?style=social)
![GitHub forks](https://img.shields.io/github/forks/rgglez/music-drums?style=social)

A simple drum kit built in response to a question from Michel Geiss about instrument building with Claude Code.

## Screenshot

![music-drums](screen.webp)

## Sounds

- Bass drum.
- Snare drum.
- Hi-hat.
- Cymbal.
- Cowbell.
- Quijada.
- Clave.

## Build with Make

### Requirements

- `g++`
- `make`
- `pkg-config`
- Qt 6 development packages (`Qt6Core`, `Qt6Gui`, `Qt6Widgets`)
- PipeWire development package (`libpipewire-0.3`)

### Build

```bash
make
```

### Run

```bash
make run
```

### Cleanup

```bash
make clean      # removes build artifacts
make distclean  # removes build artifacts + DrumBox binary
```

## Usage

Just hit the pads with the mouse.

## License

Copyright 2026 Rodolfo González González.

Licensed under [Apache License 2.0](LICENSE).

