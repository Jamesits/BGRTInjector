# BGRTInjector

Changes the boot screen image on a UEFI computer.

## Requirements

* UEFI firmware (some old EFI firmwares might be supported as well)

## Installation

Run `BGRTInjector.efi` either manually or automatically before your OS loads. You need to disable Secure Boot or sign it on your own, obviously.

### Installation on rEFInd

Put a release build of `BGRTInjector.efi` (driver mode) into `ESP:\EFI\refind\drivers_%ARCH%` and reboot.

### Installation without 3rd-party bootloader

Put a release build of `BGRTInjector.efi` (Windows loader mode) into your ESP volume and set it as the default loader. This can be achieved by putting it to `ESP:\EFI\boot\bootx64.efi`. (Firmware on some devices will load Windows first which is a violation of the UEFI standard. You might need to manually register a UEFI boot entry on these systems.)

## Usage

### Customize the image

To customize the image, put a 24-bit BMP image named `boot_image.bmp` into the root folder of your ESP volume (where `BGRTInjector.efi` lives).

### Change the built-in default image

Convert a 24-bit BMP image to C-style array initializer, replace `default_boot_image.bmp.inc` and compile. To do the convertion you can use [bin2array](https://github.com/Jamesits/bin2array):

```shell
python3 bin2array.py -O default_boot_image.bmp.inc your_image.bmp
```

## Building

Flags:

* `_DEBUG`: debug output and more pauses to see the log on the screen
* `LOAD_WINDOWS`: Windows loader mode. It will automatically search for `EFI\Microsoft\Boot\bootmgfw.efi` and start it after BGRT table has been injected

### Windows

Requirements:

* Visual Studio 2017 or higher
* C++ desktop development tools
* MSVC C++ build tools (for architectures you need)
* MSVC C++ Spectre-mitigated libs (for architectures you need)
* QEMU

Open `BGRTInjector.sln` in Visual Studio and click Build Solution. 