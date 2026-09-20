# 3DS Music Player
> A minimal, Spotify-inspired homebrew audio player engineered for Nintendo 3DS systems.

Designed with simplicity, portability, and performance in mind. **3DS Music Player** turns your Nintendo 3DS into a dedicated portable audio platform with background playback support, seamless scene switching, and native multi-format audio decoding.

---

## Key Features

* **Background Audio Engine:** Native `APT` integration allowing continuous playback even with the console lid fully closed.
* **Dual-Scene Architecture:** Instantly switch between an intuitive **Playlist View** and a clean **Player UI** using `SELECT`.
* **Multi-Format Support:** Plays `.mp3`, `.ogg`, and `.wav` audio files out of the box with zero external conversion required.
* **On-the-Fly Track Control:** Skip forward or backward through your library at any time using `L` / `R` shoulder buttons.
* **Automatic Title Sanitization:** Automatically strips file extensions (`.mp3`, `.ogg`, `.wav`) for a clean, distraction-free display interface.
* **Hardware Optimized:** Renders exclusively on the bottom touchscreen (`GFX_BOTTOM`) using `citro2d`, preserving battery life and maintaining a rock-solid 60 FPS on all 3DS hardware models.

---

## Controls

| Button | Action |
| :---: | :--- |
| `A` | Play selected track |
| `Y` | Pause / Resume current track |
| `X` | Toggle Loop Mode (ON / OFF) |
| `L` | Previous track |
| `R` | Next track |
| `SELECT` | Toggle between Playlist and Player UI |
| `DPAD Up / Down` | Navigate playlist entries |
| `START` | Exit application |

---

## SD Card Structure

Before launching the application, create a directory named `music` in the root of your SD card and place your audio tracks inside:

sdmc:/music/music.wav, music.mp3 or music.ogg
---

## Building from Source

### Prerequisites
* [devkitPro](https://devkitpro.org/) with `devkitARM` toolchain installed.
* 3DS development libraries: `libctru`, `citro3d`, `citro2d`, `SDL`, and `SDL_mixer`.

### Compilation
Clone the repository and compile using `make`:

```bash
# Clone the repository
git clone [https://github.com/3DS-Screen-Breaker/3DS-Music-Player.git](https://github.com/3DS-Screen-Breaker/3DS-Music-Player.git)
cd 3DS-Music-Player

# Build executable
make clean && make
