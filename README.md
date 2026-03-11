# Weird Shape Win32 App

Pure C Win32 desktop app with an oval Win32 window and a native close button.

## Files

- `main.c` - Win32 API app source.
- `build.bat` - Builds with the bundled `w64devkit`, MSVC, or MinGW-w64 GCC.

## Build

From a Windows terminal in this folder:

```bat
build.bat
```

This creates `weirdshape.exe`.

## Run

```bat
weirdshape.exe
```

## Controls

- Left mouse drag - move the window.
- Native `CLOSE` button - close the app.
- Right click - close the app.
- `Esc` - close the app.

## Notes

- The window shape is applied with `SetWindowRgn` using an ellipse region.
- The current app uses the default window background and no custom painting.
