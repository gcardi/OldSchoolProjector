<h1 align="center">Old Slide Projector</h1>

<p align="center">
  <em>An old-school slide projector for your desktop — and for the big screen.</em>
</p>

<p align="center">
  Relive the ritual of the analog carousel projector: dim the room, load a tray of
  photos and click through them one by one — complete with the
  <strong>mechanical clunk</strong> of the slide change and the
  <strong>warm hum</strong> of the cooling fan.
</p>

<p align="center">
  <img src="docs/assets/images/Animated.gif" alt="Old Slide Projector in action" width="760">
</p>

> **Works with every image format your Windows codecs can read** — JPG and PNG out of
> the box, and **Canon / Nikon RAW** (CR2, CR3, NEF, ARW, DNG …) and more as soon as the
> matching WIC codec is installed — and **auto-rotates every shot** from its EXIF
> orientation, so those sideways phone photos always show up straight — in the thumbnails
> and on screen.

## Overview

Old Slide Projector turns any Windows PC into a nostalgic slideshow machine. On the
surface it is deliberately simple — a strip of thumbnails, a **Prev** and a **Next**
button — but underneath it is built to stay smooth and responsive with photo libraries of
**many thousands of images**, to drive a **second display** at full screen, and to get
every picture's orientation and framing right without you having to think about it. Easy to
use, quietly more capable than it looks.

## Project onto a real screen

The projector is not confined to your desktop. Connect a **second monitor, a TV, or an
actual video projector** to a spare video output of your PC, and Old Slide Projector will
show your photos **full screen on that additional device** while you keep the controls on
your main screen. Aim it at a projector in a darkened room and you get exactly the effect of
the old mechanical carousel: pictures thrown large on the wall, one satisfying *clunk* at a
time.

- **On a display or projector**, the **aspect ratio is detected automatically** from the
  selected output device — no setup, it simply fills that screen correctly.
- **In windowed mode**, where there is no output device to measure, you choose the aspect
  ratio yourself (**4:3, 16:9 or 16:10**) from the drop-down, so the projection "canvas"
  always matches the look you want.

## Key features

- **Full-screen projection on a second display** — send the show to a monitor, TV or
  projector on another video output while the controls stay on your main screen.
- **Automatic aspect ratio** from the selected output device, or a ratio you pick yourself
  (4:3 / 16:9 / 16:10) when projecting to a window.
- **Thumbnail strip** with the current slide highlighted and kept centered, so you always
  see what came before and what comes next; click any thumbnail to jump straight to it.
- **Authentic "old school" feel** — the mechanical slide-change sound and the cooling-fan
  hum, with independent volume sliders and a sliding transition between slides.
- **Any WIC-supported format**, decoded through the Windows Imaging Component (WIC) for
  accurate color. JPG and PNG work out of the box; the list of accepted formats is read
  from the codecs installed on your PC, so adding a codec pack (for example the
  **Canon / Nikon RAW** camera codecs — CR2, CR3, NEF, ARW, DNG …) makes those files load
  too, automatically and with no update to the program.
- **Automatic EXIF rotation** — portraits are turned the right way up automatically, in the
  thumbnails and in the projection alike.
- **"Contain" framing** — every photo is shown in full (letterboxed) with no cropping.
- **Built-in test pattern (monoscope)** to line up and calibrate the target screen.
- **Image controls** — keep aspect ratio, clipping, scaling and a vintage vignette effect.
- **Pictures from any folder**, with optional recursive search through sub-folders.
- **Keyboard friendly** — Page Up / Page Down move between slides even when the projector
  window has focus and the main window is tucked away in the tray.
- **Lives in the tray**, with minimize-to-tray and optional auto-start.

## Using it

- Set the **Pictures path** (type it or browse to it) and, if your photos are spread across
  sub-folders, enable **Recursive pictures search**.
- Choose the **Display**: a connected monitor / TV / projector (aspect ratio auto-detected),
  or a **Window** with a 4:3, 16:9 or 16:10 ratio of your choice.
- Press **Start** to open the projector and **Stop** to close it. **Show / Hide** toggles the
  projector window, **Monoscope** shows a calibration test card, and **Autofit** re-fits the
  canvas to the screen.
- Move between slides in whichever way suits you:
  - **Page Up** — previous slide
  - **Page Down** — next slide
  - **Click a thumbnail** — go to that photo (a neighboring one slides across by a single
    step, a distant one jumps straight there), with the mechanical sound and animation.
- Set the mood with the **Vignetting** switch, the **Mechanism sound** and **Fan noise**
  sliders, and the **Keep aspect ratio / Clipping / Scaling** controls.

## Built for big libraries

Old Slide Projector is designed to stay responsive on folders holding **thousands upon
thousands of images**. Thumbnails are produced on a **background worker thread**, so the
interface never freezes while pictures decode. Loading is **lazy**: only the thumbnails
around the visible part of the strip are generated (plus a small look-ahead margin for
smooth scrolling), and every result is **cached** — so scrolling back is instant and the app
never tries to chew through an entire huge folder at once. The full-resolution picture for
the projection itself is always loaded on demand.

## Built with

C++ · Embarcadero C++Builder (FireMonkey / FMX) · Windows · Windows Imaging Component (WIC)
