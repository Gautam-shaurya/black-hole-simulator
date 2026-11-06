# Black Hole Simulator

A real-time, interactive 3D black hole written in **C** with **OpenGL 3.3** and **GLFW**.
Light is ray-marched around a Schwarzschild black hole, so you see gravitational lensing,
a glowing photon ring, a turbulent rotating accretion disk with Doppler brightening,
a lensed starfield and nebula, and thousands of orbiting particles.

![Preview](docs/preview.png)

## Features
- Per-pixel ray marching with approximate Schwarzschild light bending (photon sphere at 1.5 rs, shadow radius 2.6 rs)
- Accretion disk with 16 differentially rotating layers, heat-haze turbulence and relativistic Doppler / redshift colouring
- Procedural starfield (thousands of stars) and nebula, bent by the lensing
- GPU-friendly particle system: orbiting and inward-spiralling particles with trails
- HDR rendering with bloom, ACES tone mapping, colour grading, vignette and dithering (no banding)
- Minimal, thin-line interface: live controls and data, collapsible "learn more" section
- Cinematic view: hides every panel and text, slow automatic orbit, letterbox
- Friendly error messages and an `error.log` file if something goes wrong

## Quick start (Windows 10/11, no programming knowledge needed)
1. Click **Code > Download ZIP**, then right-click the ZIP and choose **Extract All...**
2. Open the extracted folder and double-click **`build_windows.bat`** (one time only, needs internet, takes a few minutes).
   It downloads a free portable compiler and the official GLFW into the folder and builds `BlackHoleSimulator.exe`.
   If Windows says "Windows protected your PC", click **More info > Run anyway**.
3. Double-click **`BlackHoleSimulator.exe`**. Next time, just step 3.

`launch.bat` is a backup launcher that checks the files and shows a helpful message if something is missing.
Nothing is installed on your system; delete the folder to remove everything.

### Build on Linux / macOS
Install GLFW (`sudo apt install libglfw3-dev` or `brew install glfw`), then:
```
cc -std=gnu99 -O2 -Iinclude src/*.c -o BlackHoleSimulator -lglfw -lm
./BlackHoleSimulator
```
Run it from the project folder so the `shaders/` folder is found. (The Linux/macOS build is untested on real hardware.)

## Controls
| Input | Action |
|---|---|
| Left mouse drag | Orbit the camera |
| Mouse wheel | Zoom |
| Middle mouse drag | Pan |
| W A S D Q E | Fly forward / left / back / right / down / up |
| R | Reset the camera |
| Space | Pause / resume |
| C | Cinematic view (hides all UI) |
| H | Hide / show panels |
| Esc | Quit |

The camera cannot enter the exclusion sphere around the hole (2.5 rs).

## Requirements
- Windows 10/11 64-bit
- A graphics card and driver supporting **OpenGL 3.3** (any normal PC from the last decade)
- Not supported: Remote Desktop and most virtual machines

## Troubleshooting
| Problem | Fix |
|---|---|
| "Could not open an OpenGL 3.3 window" | Update your graphics driver (Windows Update, or NVIDIA / AMD / Intel website). |
| "A program file could not be found" | Extract the whole ZIP; the `shaders` folder must be next to the exe. |
| "A graphics program (shader) failed to compile" | Send the contents of `error.log` in an issue; it names the file and line. |
| `build_windows.bat` fails | Check your internet connection / firewall and run it again; details are in `build_error.log`. |
| Low frame rate | Make the window smaller; the ray marcher is GPU-heavy. |

## Project layout
```
src/       C source (main, camera, black hole, disk, starfield, particles, post-processing, UI, physics, GL loader, error log)
include/   headers
shaders/   GLSL 330 shaders (ray marcher, particles, bloom/tonemap, UI)
docs/      preview image
build_windows.bat / build_windows.ps1   one-click Windows build
launch.bat                              backup launcher
```
No third-party code is bundled. GLFW is downloaded at build time from its official release; OpenGL
functions are loaded by a small built-in loader (`src/gl_loader.c`); the UI uses a built-in thin vector font.

## About the physics
All values in the interface are in **simulation units (SU)**, not real astronomical measurements.
Lensing uses the Schwarzschild light-bending acceleration `a = -1.5 h^2 r / |r|^5` (in units of the
Schwarzschild radius). It reproduces the photon sphere, the shadow size and the weak-field deflection,
but it is **an approximation, not a full general-relativity simulation** (no spin, thin emitting disk,
artistic colours).

## Status
The application renders and its controls were tested headlessly on a software OpenGL 3.3 renderer and
run on a real AMD GPU on Windows by the author. Please open an issue if you find a driver-specific problem.
