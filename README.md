<h1 align="center">
travyboard
</h1>

## A corne style keyboard with the Pico SDK and TinyUSB

A personal project demonstrating how to build a keyboard using the
Raspberry Pi Pico microcontroller in c++. Intent on expanding to create more powerful
keyboards with more features.

### Photos
<div style="display: flex; justify-content: space-around;">
  <img src="./assets/IMG_1684.jpeg" alt="First Photo" style="width: 45%; transform: rotate(-90deg);">
  <img src="./assets/IMG_1680.jpeg" alt="Second Photo" style="width: 45%;">
</div>

### Features

- 3d printed case
- 3x6 matrix with 3 button thumb clusters and 2 mode keys
- Split one way communication
- Full keymap controlled by the master pico
- 4 layers
- ws2812 rgb leds on every key

### TODO

- [ ] Create modular halves (gaming optimized, cad optimized, etc). I would consider the first iteration vim/coding/desktop optimized
- [ ] Add rotary encoder support
- [ ] Add OLED support
- [ ] Add mode changing support
- [ ] Add led firmware and effects
- [ ] Revise case to show RGB better
- [x] Add qmk support
    - [ ] Add via support

### Requirements for building

- Two Raspberry Pi Picos. I have the zeroes on the actual PCB.
- Cmake and gcc/g++ installed. May want to set up wsl2 on Windows.
- Pico SDK [GitHub Repo](https://github.com/raspberrypi/pico-sdk)

### Setup

1. Clone the Pico SDK and update the submodules

```sh
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

2. Set an environment variable so cmake can find the sdk

```sh
export PICO_SDK_PATH=`<pico sdk path>`
```

3. Clone this repository:

```sh
git clone https://github.com/travmonkey/travyboard.git
cd travyboard
```

4. Create build folder and build the project:

```sh
cd firmware
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. # Export the compile commands for clangd to see
make
```

5. Mount and flash both picos with the master and slave compiled .uf2 code.

```sh
mkdir ~/rpi-pico
sudo fdisk -l # Find the pico device
sudo mount /dev/sdx1 /home/`<user>`/rpi-pico
sudo cp src/master.uf2 /home/`<user>`/rpi-pico/
```

Alternativly you can modify the script I wrote to include your own directories and username
