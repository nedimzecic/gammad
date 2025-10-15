# gammad
A lightweight Linux utility for applying gamma correction to display outputs using DRM/KMS. This tool allows you to adjust the RGB color channels of your display, commonly used for blue light filtering or color temperature adjustments. This was made mainly to provide Night Light feature to Cosmic DE

## Installation

### Install Dependencies
```bash
sudo apt-get install libdrm-dev build-essential
```

### Build
```bash
make
```

### Install
```bash
sudo make install
```

## Usage
```bash
gammad <card_name> <red_scale> <green_scale> <blue_scale>
```

### Arguments
- `card_name`: Name of the DRM card (e.g., card0, card1)
- `red_scale`: Red channel scaling factor (0.0 to 1.0)
- `green_scale`: Green channel scaling factor (0.0 to 1.0)
- `blue_scale`: Blue channel scaling factor (0.0 to 1.0)

### Example
```bash
gammad card1 1.0 0.8 0.7
```

## Finding Your Graphics Card
To list available DRM devices:
```bash
ls /dev/dri/
```

## Cosmic DE
gammad should be injected right after shebang in /usr/bin/cosmic-greeter-start:
```
#!/bin/sh
gammad card1 1.0 0.8 0.7
rm -rf /run/cosmic-greeter/cosmic/com.system76.CosmicSettingsDaemon/v1/* > /dev/null 2>&1
exec cosmic-comp cosmic-greeter > /dev/null 2>&1
```
