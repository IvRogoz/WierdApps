# Weird Shape Win32 App

Pure C Win32 experiments for non-rectangular windows.

## Folders

- `basic/` - simple oval window with a native `CLOSE` button.
- `drivenbyimage/` - shape generated from `shape.bmp` using `SetWindowRgn`.
- `w64devkit/` - optional bundled toolchain used by the local build scripts.

## Build

From a Windows terminal, build inside the example folder you want:

```bat
cd basic
build.bat
```

or:

```bat
cd drivenbyimage
build.bat
```

## Run

## Examples

- `basic/weirdshape.exe` - default background oval window.
- `drivenbyimage/imagedshape.exe` - window region driven by image pixels.
