# MidiFoo
A bluetooth/USB MIDI controller/footswitch with a built-in guitar tuner, powered by STM32F4.

The idea is to get an inexpensive stompbox that meets all the needs of a musician in a live perfomance. All the parts of this projects are found in the open-source GPL license which means free to use and modify, you can build your own midifoo with the instructions below.
<p>
  <img src="/img/midifoo-tuner.jpg" width="500">
</p>

# Features

- **USB MIDI (Class-Compliant):**\
Automatically recognized by computers and compatible audio devices.

- **Bluetooth MIDI:**\
Low-latency wireless communication for pratical use.

- **Built-in Guitar Tuner:**\
Audio input and output for instrument signal.

- **Lots of MIDI Note Banks**.

- **12+ Hours of Inenterrupt Use in Battery.**

- **Dual Function Switches:**\
Quick and Long Press triggers different MIDI Messages.
- **Expression Pedal Input**
- **MIDI Potentiometer**

# PCB and Case
The gerber files for the PCB and .stl model for the 3d printed case can be found in this repo: https://github.com/rbmannchued/midifoo-kicad

# Build and Flash Code
Clone the repo with submodules:
```
git clone --recurse-submodules https://github.com/rbmannchued/midifoo.git
```
Build the binaries:
```
cd src/ 
make
```
Flash the firmware using ST-Link:
```
make flashbin
```
# Use
For usb midi just connect a cable to your pc, is plug and play. For bluetooth you will need something like Hairless MIDI ( https://github.com/projectgus/hairless-midiserial ) or ttymidi ( http://www.varal.org/ttymidi/ ). Works on both Linux and Windows. 
