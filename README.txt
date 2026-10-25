ILOT14 ISLAND  v0.4.0   (Switch homebrew NRO, C++ / SDL2 / libnx)

An original, cursed island life sim starring the Ilot14 residents. 26 characters live in the 14 building.

BUILD
  Put these files in your repo (build.yml goes to .github/workflows/build.yml) and let GitHub Actions build the NRO.
  Install location on the SD card: /switch/ilot14_island/ilot14_island.nro  (saves in the same folder: save.dat)

FIRST RUN
  A setup screen asks for your name, clock mode (real device date/time or fast), curse level, volume and a starter pack,
  then explains the bubbles and controls. NEW GAME re-runs it. The header shows today's real date and time.
  The NRO icon is icon.jpg (256x256) in the same folder as the Makefile.

CONTROLS
  Touch: tap people, tap the bubbles above their heads, hold + drag to pick someone up.
  Drop a person on another person to introduce them (or to help two stuck friends make up).
  In the ROOMS tab, drag a person onto a room to move them. In the BAG tab, drag an item onto a person.
  Gamepad: left stick moves the pointer, A = tap/hold, B = back, L/R = previous/next room, + = settings, Y = photo (map).

BUBBLES
  yellow ! with item tag  = wants that item     blue ...  = sad (give food/gifts)     orange +  = wants to be friends
  purple triangles        = stuck with someone  green play = wants a minigame         Zz        = dreaming (tap to read it)
  pink heart              = Gabriel's very awkward crush on Dalia (it never goes well)

SOUNDTRACK
  Built-in procedural music. To replace it, put WAV files in /switch/ilot14_island/music/ :
  menu.wav  day.wav  night.wav  minigame.wav     (any sample rate; they loop)

PHOTOS: /switch/ilot14_island/photos/   (BMP)

EDIT THE CAST: source data lives in data.hpp (looks, ages, personalities, dialogue). Looks are placeholders until real art is supplied.
