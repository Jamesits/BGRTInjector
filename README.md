# BGRTInjector

Changes the boot screen image on a UEFI computer.

This is a remake of [Ben Wang](https://github.com/imbushuo)'s [`windows_custom_loader.efi`](https://archive.org/details/windows_custom_loader_by_ben_wang), the source code of which is long lost. Several incompatiblities with non-Apple UEFI implementations are addressed, and you can now replace the logo without recompiling the whole program.

## Requirements

* UEFI firmware (some old EFI firmwares might be supported as well)
* An operating system that reads BGRT table (reads: Windows 8.1 or later)

## Installation

Run `BGRTInjector.efi` either manually or automatically before your OS loads. You need to disable Secure Boot or sign it on your own, obviously.

### Installation on rEFInd

Put a release build of `BGRTInjector.efi` (driver mode) into `ESP:\EFI\refind\drivers_x64` and reboot.

### Installation without 3rd-party bootloader

Put a release build of `BGRTInjector.efi` (Windows loader mode) into your ESP volume and set it as the default loader. This can be achieved by putting it to `ESP:\EFI\boot\bootx64.efi`. (Firmware on some devices will load Windows first which is a violation of the UEFI standard. You might need to manually register a UEFI boot entry on these systems.)

## Usage

### Customize the image

To customize the image, put a **24-bit** (other modes and formats are not supported) BMP image named `boot_image.bmp` into the root directory of your ESP volume (where `BGRTInjector.efi` lives). If BGRTInjector complains about image file size incorrect, please open the BMP image in `mspaint.exe` then save it to fix its header.

The image will be displayed on the center of the screen, 1:1 pixel aligned. If the image is larger than the default UEFI GOP resolution, it will not be displayed. 

### Change the built-in default image

Convert a 24-bit BMP image to C-style array initializer, replace `default_boot_image.bmp.inc` and compile. To do the convertion you can use [bin2array](https://github.com/Jamesits/bin2array):

```shell
python3 bin2array.py -O default_boot_image.bmp.inc your_image.bmp
```

### Security

Loading untrusted image into memory is dangerous. BGRTInjector only reads the image file from the volume (partition) it lives in, and ESP partition is usually protected under end-user accessible operating systems, so we can assume only a system administrator or [a evil maid](https://en.wikipedia.org/wiki/Evil_maid_attack) can load an evil image. Additionally BGRTInjector does some basic sanity checks on the image file, but it is still prone to specially crafted evil images. 

If you are not signing your own Secure Boot keys, using BGRTInjector means Secure Boot will be unavailable. In Windows loader mode, BGRTInjector does not verify the authenticity of the target bootloader. 

### FAQ

#### Stuck at loading boot image from disk

You have too many hard disks or [the NTFS driver comes with rEFInd is causing a hang](https://www.rodsbooks.com/refind/drivers.html#selecting). Delete `ESP:\EFI\refind\drivers_x64\ntfs_x64.efi` and other filesystem drivers if you don't need them. 

#### Image is not centered

This typically happens when you are using driver mode and your firmware does not set GOP resolution to the max available value. All the drivers are launched prior to rEFInd setting the GOP resolution, so BGRTInjector would read a smaller screen resolution.

The most simple solution:

* Remove BGRTInjector from `drivers_x64` folder
* Download a Windows loader mode BGRTInjector and put it under `ESP:\`
* Hard code the resolution you need in rEFInd's config file
* Either tell rEFInd not to switch to text mode before loading OS, or disable graphics mode (full screen logo display) in your BIOS

Here's a rEFInd config from my computer for reference.

```
# Your resolution may vary
resolution 3840 2160

menuentry "Windows 10 (BGRT Injected)" {
    # do not switch to text mode when booting; otherwise screen resolution will still be incorrect
    graphics on

    icon \EFI\refind\icons\os_win8.png

    # use BGRTInjector Windows loader mode 
    loader \BGRTInjector.efi
}
```

## Building

Flags:

* `_DEBUG`: debug output and more pauses to see the log on the screen.
* `LOAD_WINDOWS`: use Windows loader mode if set, otherwise use driver mode. In Windows loader mode, it will automatically search for `EFI\Microsoft\Boot\bootmgfw.efi` and start it after BGRT table has been injected. In driver mode, it quits after BGRT table has been injected.
* `VERTICAL_ALIGN_RATIO` and `HORIZONTAL_ALIGN_RATIO`: a float number between 0 and 1 to indicate where to put the image on the screen.

### Windows

Requirements:

* Visual Studio 2017 or higher
* C++ desktop development tools
* MSVC C++ build tools (for architectures you need)
* MSVC C++ Spectre-mitigated libs (for architectures you need)
* QEMU

Open `BGRTInjector.sln` in Visual Studio and click Build Solution. 