# BGRTInjector

Changes the boot screen image on a UEFI computer.

## Requirements

* UEFI firmware (some old EFI firmwares might be supported as well)

## Usage

Run `BGRTInjector.efi` either manually or automatically before your OS loads.

### rEFInd

Put a release build of BGRTInjector.efi into `ESP:\EFI\refind\drivers_%ARCH%` and reboot.

## Building

### Windows

Requirements:

* Visual Studio 2017 or higher
* C++ desktop development tools
* MSVC C++ build tools (for architectures you need)
* MSVC C++ Spectre-mitigated libs (for architectures you need)
* QEMU

Open `BGRTInjector.sln` in Visual Studio and click Build Solution. 