<div align="center">

# Stemlab PrintFarm Orca Slicer

**A fork of OrcaSlicer integrated with the Stemlab PrintFarm platform**

[![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![Based on OrcaSlicer](https://img.shields.io/badge/based%20on-OrcaSlicer-orange?style=flat-square)](https://github.com/OrcaSlicer/OrcaSlicer)
[![License](https://img.shields.io/badge/license-AGPL--3.0-blue?style=flat-square)](LICENSE.txt)

</div>

---

## Overview

Everything OrcaSlicer does — plus a built-in Print Farm workflow. Sign in to your Stemlab PrintFarm directly from the slicer, pick a printer, and send the job without managing device IPs or API keys manually.

---

## What This Fork Adds

- **In-app login** — Print Farm sign-in embedded in the OrcaSlicer window on every launch
- **Automatic printer sync** — farm printers appear in the printer dropdown as `Name (Farm)` after login
- **Zero-touch credentials** — a short-lived upload token is minted on login and revoked on exit; no API key management needed
- **Upload to Farm** — sends the sliced file to the selected printer via the farm backend (Bambu over MQTT, Snapmaker over Moonraker)
- **Jobs dashboard** — view the queue and cancel jobs from within the slicer

---

## How It Connects

```
OrcaSlicer ──login──▶ PrintFarm API ──▶ printers / jobs
           ──mint token──▶ ephemeral slicer_upload key (memory only)
           ──Upload to Farm──▶ slicer-proxy ──▶ Bambu MQTT / Snapmaker Moonraker
```

All Print Farm code lives under `src/slic3r/{Utils,GUI}/PrintFarm/` and is marked with `// >>> PRINTFARM` on upstream files — making rebases straightforward.

---

## Build

```bash
# Linux
./build_linux.sh

# macOS
./build_release_macos.sh

# Windows
build_release_vs2022.bat
```

See [`docs/printfarm-integration-plan.md`](docs/printfarm-integration-plan.md) for architecture details.

---

<div align="center">

Built at **Satit Chulalongkorn University STEM Lab**, Bangkok Thailand

</div>
