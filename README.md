# Zubnetic

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

### Requirements
- JUCE Framework (tested with JUCE 7.x)
- Visual Studio 2019 or newer (Windows)
- Xcode (macOS)

### Build Instructions

1. Clone this repository
2. Open `zubnetic.jucer` in Projucer
3. Click "Save and Open in IDE"
4. Build the VST3 target
5. Copy the built .vst3 to your VST3 folder

## License

GPL-3.0 - See LICENSE file for details

## Support

If you find this useful, consider [buying me a coffee or a bag of weed](https://buymeacoffee.com/zubwu)!

## Contributing

Contributions welcome! Feel free to open issues or submit pull requests.
