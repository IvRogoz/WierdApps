# Animated Dog Mascot

Small Win32 desktop mascot built from `Dog_medium.png`.

## Sprite sheet layout

Rows:

1. Bark
2. Walk
3. Run
4. Sit Transition
5. Idle Sit
6. Idle Stand

Detected frame grid:

- frame size: `60x38`
- scale: `3x`
- row frame counts: `4, 6, 5, 3, 4, 4`

## Behavior

- Starts near the bottom-right of the work area.
- Randomly idles, walks, runs, sits, and barks.
- Double click to bark immediately.
- Drag with left mouse.
- Right click to close.

## Build

```bat
build.bat
```

## Run

```bat
dog_mascot.exe
```
