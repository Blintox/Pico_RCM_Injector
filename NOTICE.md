# Third-Party Notices

This repository includes code and payload data derived from third-party open source projects.

## Hekate Payload

`include/payload.h` is a generated C header containing the Hekate RCM payload bytes.

- Upstream project: `CTCaer/hekate`
- Source repository: https://github.com/CTCaer/hekate
- Release used: `hekate v6.5.2 & Nyx v1.9.2`
- Release page: https://github.com/CTCaer/hekate/releases/tag/v6.5.2
- Binary embedded locally: `hekate_ctcaer_6.5.2.bin`
- License text: `third_party/hekate/LICENSE`

The generated header is included for convenience so this project builds into a ready-to-flash injector. If you want to use a different payload, regenerate it with:

```powershell
python tools/bin2payload_h.py C:\path\to\payload.bin include\payload.h
```

## SAM Fusee Launcher Internal

The RP2040 injector sequence was ported from public microcontroller Fusee launcher prior art, especially `blockfeed/sam-fusee-launcher-internal`.

- Upstream project: `blockfeed/sam-fusee-launcher-internal`
- Source repository: https://github.com/blockfeed/sam-fusee-launcher-internal
- Reference file: `main/main.ino`
- License text: `third_party/sam-fusee-launcher-internal/LICENSE`

## This Project

The Pico/RP2040 firmware and tooling in this repository are licensed under the top-level `LICENSE`, except where third-party notices state otherwise.
