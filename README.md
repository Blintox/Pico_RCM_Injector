# Pico RCM Injector

This is a Raspberry Pi Pico / RP2040 firmware scaffold for using the Pico as a USB host that detects an NVIDIA APX/RCM device (`0955:7321`) and sends an embedded RCM payload.

It is modeled after the shape of `fusee-alpestre`, but not the implementation. `fusee-alpestre` builds a Raspberry Pi Linux image and runs a launcher from Linux; this project is bare-metal Pico SDK firmware using TinyUSB host mode.

## Instructions

1. Flash latest .uf2 to pico from releases
2. Power pico with 5v through VBUS and Ground (must be VBUS and 5v)
3. Put device in RCM mode and connect powered pico through usb-c
4. Afer Payload Delivery disconnect pico

## Lawful Use Only

This project is provided only for lawful homebrew, recovery, repair, research, and interoperability work on hardware you personally own or are explicitly authorized to service.

Do not use this project to pirate games, bypass payment, defeat online service rules, run unauthorized copies of copyrighted works, distribute console keys, distribute copyrighted firmware, or help anyone else do those things.

You are responsible for understanding and following the laws, licenses, warranty terms, and platform rules that apply where you live. This repository does not grant permission to circumvent copyright protection systems or use third-party copyrighted material without authorization.

If you are unsure whether your intended use is lawful, do not use this project until you have checked the applicable rules or obtained proper legal advice.

This repository includes a generated Hekate payload header. See `NOTICE.md` and `third_party/` for source, attribution, and license details.

## Hardware Notes

The Pico must act as the USB host and provide VBUS to the Switch-side USB connection. A normal Pico plugged into a PC over micro-USB is a USB device, not a host.

Minimum wiring for a standalone injector:

- Pico USB `D+` and `D-` to the Switch USB-C cable/device connector data pair.
- Stable 5 V to the target connector VBUS through a current-limited source.
- Common ground between Pico, 5 V supply, and USB connector.
- Pico powered from `VSYS` or `VBUS`, depending on your power design.

Do not connect two powered USB hosts together. Use a known-good USB-C data cable and a current-limited 5 V source.

## Build

Install the Raspberry Pi Pico SDK and ARM GCC toolchain, then set `PICO_SDK_PATH`.

```powershell
python tools/bin2payload_h.py C:\path\to\payload.bin include\payload.h
.\build.ps1
```

`include/payload.h` is generated from the embedded payload. Use `include/payload.example.h` as the placeholder/reference format if you want to replace it.

Flash `build/pico_rcm_injector.uf2` by holding `BOOTSEL` while plugging the Pico into your PC, then copying the UF2 to the `RPI-RP2` drive.

This project generates the UF2 locally with `tools/bin2uf2.py`, so it does not require Pico SDK's optional `picotool` helper.

## Runtime Behavior

- Slow blink: waiting for an APX/RCM USB device.
- One long blink after attach: payload sequence completed.
- Repeating short blink count: error code.

Error blink codes:

- 1 blink: no payload embedded.
- 2 blinks: APX endpoints could not be opened.
- 3 blinks: reserved for device ID read failure; current builds continue past this check.
- 4 blinks: payload bulk write failed.
- 5 blinks: final trigger/control transfer failed.

UART logging is enabled because USB stdio is disabled while the RP2040 USB peripheral is in host mode.

## Project Layout

- `src/main.c`: TinyUSB host loop, APX detection, LED status.
- `src/rcm_injector.c`: endpoint setup, transfer buffering, launch sequence.
- `include/payload.h`: generated payload array, initially a placeholder.
- `tools/bin2payload_h.py`: converts a `.bin` payload into a C header.

## Sources And Prior Art

- [kleo/fusee-alpestre](https://github.com/kleo/fusee-alpestre)
- [blockfeed/sam-fusee-launcher-internal](https://github.com/blockfeed/sam-fusee-launcher-internal)
- [Raspberry Pi Pico SDK TinyUSB host support](https://www.raspberrypi.com/documentation/pico-sdk/third_party.html#tinyusb_host)
- [TinyUSB USB concepts](https://docs.tinyusb.org/en/latest/reference/usb_concepts.html)

## License

GPL-3.0-only. The launch sequence is derived from GPL microcontroller implementations of the public Fusee launcher flow.
