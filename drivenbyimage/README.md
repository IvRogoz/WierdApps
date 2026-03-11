# Driven By Image

Win32 example where the window shape comes from a bitmap image.

## How it works

- Place a `shape.bmp` file in this folder.
- Magenta (`RGB(255, 0, 255)`) is treated as transparent.
- Every other pixel becomes part of the window region.
- A sample `shape.bmp` is included.

## Build

```bat
build.bat
```

## Run

```bat
imagedshape.exe
```

## Notes

- This sample uses `SetWindowRgn` built from bitmap pixels.
- `shape.bmp` should be a 24-bit or 32-bit BMP for predictable results.
- The app paints the bitmap and overlays a native `CLOSE` button.
