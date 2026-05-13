# MidiFoo

<p align="center">
  <img src="/img/midifoo-tuner.jpg" width="600" alt="MidiFoo device showing built-in guitar tuner">
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/github/license/rbmannchued/midifoo?color=blueviolet" alt="License"></a>
  <img src="https://img.shields.io/badge/platform-STM32F4-blue" alt="Platform">
  <img src="https://img.shields.io/badge/MIDI-USB%20%7C%20Bluetooth-green" alt="MIDI">
  <img src="https://img.shields.io/badge/battery-12%2B%20hours-orange" alt="Battery">
</p>

---

A Bluetooth/USB MIDI controller and footswitch with a built-in guitar tuner, powered by the STM32F4.

The goal is an affordable stompbox that covers everything a musician needs in a live performance — no compromises. All parts of this project are licensed under the GPL, meaning you're free to use, modify, and build your own MidiFoo using the instructions below.

---

## Features

| Feature | Details |
|---|---|
|**USB MIDI (Class-Compliant)** | Plug-and-play with computers and compatible audio devices, no drivers needed |
|**Bluetooth MIDI** | Low-latency wireless connection for a cable-free setup |
|**Built-in Guitar Tuner** | Audio input and output for direct instrument signal processing |
|**Multiple MIDI Note Banks** | Switch between banks on the fly |
|**12+ Hours Battery Life** | Uninterrupted use for long sessions and gigs |
|**Dual-Function Switches** | Short and long press send different MIDI messages |
|**Expression Pedal Input** | Connect any standard expression pedal |
|**MIDI Potentiometer** | Real-time control over MIDI parameters |

---

## Hardware

PCB Gerber files and a 3D-printable case model (.stl) are available in the companion repository:

[rbmannchued/midifoo-kicad](https://github.com/rbmannchued/midifoo-kicad)

---

## Dependencies
- gcc-arm-none-eabi
- stlink-tools

## Build & Flash

### 1. Clone the repository (with submodules)
```bash
git clone --recurse-submodules https://github.com/rbmannchued/midifoo.git
```
### 2. Build libopencm3 STM32F4 
```bash
cd libopencm3/ && make TARGETS='stm32/f4' && cd ..
```

### 3. Build the firmware
```bash
cd src/
make
```

### 4. Flash via ST-Link
```bash
make flashbin
```

---

## Usage

### USB MIDI
Connect a USB cable to your PC, it works as a class-compliant device, no drivers or configuration needed. Works on Linux and Windows.

### Bluetooth MIDI
To use Bluetooth MIDI on a desktop, you'll need a serial-to-MIDI bridge. Recommended options:

- **[Hairless MIDI](https://github.com/projectgus/hairless-midiserial)** — cross-platform GUI tool
- **[ttymidi](http://www.varal.org/ttymidi/)** — lightweight Linux CLI alternative

---

## License

This project is open source under the [GPL license](LICENSE). You are free to use, modify, and distribute it.
