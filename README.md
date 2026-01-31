# Zubnetic

[![Buy Me A Coffee Or A Bag Of Weed](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Support-yellow?style=for-the-badge&logo=buy-me-a-coffee)](https://buymeacoffee.com/ZubWu)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue.svg?style=for-the-badge)](LICENSE)
[![Release](https://img.shields.io/github/v/release/JoseScript/Zubnetic?style=for-the-badge)](https://github.com/JoseScript/Zubnetic/releases)

An open-source stereo visualizer VST3 plugin with multi-shape rendering and frequency-reactive colors.

<img width="454" height="389" alt="zubnetic a 1" src="https://github.com/user-attachments/assets/44748d67-b52f-4956-9405-160812cd562c" />

## Features

- **Multiple visualization modes**: Lines and particles
- **Shape options**: Circle, star, square, and spiral patterns for mono content
- **Wave modulation**: Sine, triangle, square, and sawtooth wave shaping
- **Color modes**: Energy-based or FFT frequency-based coloring
- **Customizable effects**: Adjustable glow, saturation, color inversion
- **Real-time controls**: Gain, zoom, rotation, persistence, thickness, and more
- **Resizable window**: Currently fluid resolution

## Download

[Download the latest release](https://github.com/JoseScript/Zubnetic/releases)

## Usage

Load Zubnetic as a VST3 plugin in your DAW (Ableton, FL Studio, Reaper, etc.). Route audio through it to visualize your sound. All parameters are automatable.

## Building from Source

This project uses JUCE's Projucer-based build system to generate native IDE projects
### Requirements
- JUCE Framework (tested with JUCE 7.x, Projucer workflow)
- Visual Studio 2019 or newer (Windows)
- Xcode (macOS)

### Build Instructions

1. Clone this repository
2. Open `zubnetic.jucer` in Projucer
3. Export the project using Projucer (Save and Open in IDE)
4. Build the VST3 target
5. Copy or install the built .vst3 into your system's VST3 plugin directory

## License

GPL-3.0 - See LICENSE file for details

## Support

If you find this useful, consider [buying me a coffee or a bag of weed](https://buymeacoffee.com/zubwu)!

## Contributing

Contributions welcome! Feel free to open issues or submit pull requests.
