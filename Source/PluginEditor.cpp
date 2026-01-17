/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void XYscopeAudioProcessorEditor::timerCallback()
{
    renderFrame();
    repaint();
}
//===============================================================================
void XYscopeAudioProcessorEditor::renderFrame()
{
    const int N = 4096;
    if ((int)scratchL.size() != N)
    {
        scratchL.resize(N);
        scratchR.resize(N);
        points.resize(N);
    }

    const int got = processor.pullSamples(scratchL.data(), scratchR.data(), N);
    if (got < 2)
        return;

    // --- Visual auto-gain (AGC) ---
    float peak = 1.0e-6f; // avoid divide-by-zero
    for (int i = 0; i < got; ++i)
    {
        // Use mid or max of L/R; choose what "fills" best for your aesthetic
        float m = 0.5f * (std::fabs(scratchL[i]) + std::fabs(scratchR[i]));
        if (m > peak) peak = m;
    }

    // --- Global energy for colour (frame-level) ---
    float e = 0.0f;
    for (int i = 0; i < got; ++i)
    {
        float m = 0.5f * (scratchL[i] + scratchR[i]); // mid
        e += m * m;
    }
    e = std::sqrt(e / (float)got); // RMS ~ 0..1

    // Smooth it so colours don't flicker
    const float colourAttack = 0.25f;
    const float colourRelease = 0.05f;
    if (e > colourEnergySmoothed)
        colourEnergySmoothed += (e - colourEnergySmoothed) * colourAttack;
    else
        colourEnergySmoothed += (e - colourEnergySmoothed) * colourRelease;

    // Map RMS to a usable 0..1 control signal (tune these)
    float energyNorm = juce::jmap(colourEnergySmoothed, 0.02f, 0.25f, 0.0f, 1.0f);
    energyNorm = juce::jlimit(0.0f, 1.0f, energyNorm);


    // We want peak * visualGain ? desiredPeak
    // desiredPeak is in "audio units" before scale; tune by feel.
    const float desiredPeak = 0.35f; // 0..1-ish
    float targetVisualGain = desiredPeak / peak;

    // Clamp so silence doesn't blow up and loud signals don't vanish
    targetVisualGain = juce::jlimit(0.25f, 20.0f, targetVisualGain);

    // Smooth: faster attack, slower release
    const float attack = 0.25f;   // increase = faster response to quiet signals
    const float release = 0.05f;  // decrease = slower drop when signal gets loud

    if (targetVisualGain > visualGainSmoothed)
        visualGainSmoothed += (targetVisualGain - visualGainSmoothed) * attack;
    else
        visualGainSmoothed += (targetVisualGain - visualGainSmoothed) * release;

    const float zoom = processor.zoomParam ? processor.zoomParam->load() : 1.0f;
    const float gainDb = processor.gainDbParam ? processor.gainDbParam->load() : 0.0f;
    const float gain = juce::Decibels::decibelsToGain(gainDb);
    const float deg = processor.rotateDegParam ? processor.rotateDegParam->load() : 0.0f;
    const float a = juce::degreesToRadians(deg);
    const float c = std::cos(a);
    const float s = std::sin(a);

    const float persist = processor.persistParam ? processor.persistParam->load() : 0.85f;
    const float fadeAlpha = juce::jlimit(0.0f, 1.0f, 1.0f - persist);

    if (!accumulation.isValid()
        || accumulation.getWidth() != getWidth()
        || accumulation.getHeight() != getHeight())
    {
        accumulation = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    }

    juce::Graphics g(accumulation);
    g.setColour(juce::Colours::black.withAlpha(fadeAlpha));
    g.fillAll();

    auto area = getLocalBounds().toFloat();
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float scale = 0.45f * std::min(area.getWidth(), area.getHeight());

    // Draw in chunks with varying thickness and spread
    const int chunkSize = 128;
    g.setColour(juce::Colours::white);

    for (int chunkStart = 0; chunkStart < got; chunkStart += chunkSize)
    {
        int chunkEnd = std::min(chunkStart + chunkSize, got);
        int chunkLen = chunkEnd - chunkStart;

        if (chunkLen < 2)
            continue;

        // Get user controls
        const float satControl = processor.saturationParam ? processor.saturationParam->load() : 1.0f;
        const float hueControl = processor.hueShiftParam ? processor.hueShiftParam->load() : 0.0f;
        const float monoAmount = processor.monoAmountParam ? processor.monoAmountParam->load() : 0.0f;
        const float thicknessControl = processor.thicknessParam ? processor.thicknessParam->load() : 1.0f;
        const int monoShape = processor.monoShapeParam ? (int)processor.monoShapeParam->load() : 0;
        const int waveType = processor.waveTypeParam ? (int)processor.waveTypeParam->load() : 0;
        const float glowIntensity = processor.glowIntensityParam ? processor.glowIntensityParam->load() : 1.0f;  
        const float glowSize = processor.glowSizeParam ? processor.glowSizeParam->load() : 5.0f;
        const bool particleMode = processor.particleModeParam ? (processor.particleModeParam->load() > 0.5f) : false;
        const bool fftMode = processor.fftModeParam ? (processor.fftModeParam->load() > 0.5f) : false;
        const float dcOffset = processor.dcOffsetParam ? processor.dcOffsetParam->load() : 0.0f;          
        const bool invertColors = processor.invertColorsParam ? (processor.invertColorsParam->load() > 0.5f) : false;     

        // Wave shaping function for radius modulation
        auto getWaveModulation = [waveType](float phase) -> float
            {
                // phase is 0..1 representing one cycle
                switch (waveType)
                {
                case 0: // Sine (smooth)
                    return std::sin(phase * juce::MathConstants<float>::twoPi);

                case 1: // Triangle (linear ramps)
                    return (phase < 0.5f)
                        ? juce::jmap(phase, 0.0f, 0.5f, -1.0f, 1.0f)
                        : juce::jmap(phase, 0.5f, 1.0f, 1.0f, -1.0f);

                case 2: // Square (hard edges)
                    return (phase < 0.5f) ? 1.0f : -1.0f;

                case 3: // Sawtooth (ramp up, snap down)
                    return juce::jmap(phase, 0.0f, 1.0f, -1.0f, 1.0f);

                default:
                    return std::sin(phase * juce::MathConstants<float>::twoPi);
                }
            };
        // Calculate stereo width for this chunk
        float widthSum = 0.0f;
        for (int i = chunkStart; i < chunkEnd; ++i)
        {
            float diff = std::abs(scratchL[i] - scratchR[i]);
            widthSum += diff;
        }

        float stereoWidth = widthSum / (float)chunkLen;
        stereoWidth = juce::jlimit(0.0f, 1.0f, stereoWidth * 0.5f);
        stereoWidth *= (1.0f - monoAmount);

        // Chunk energy (RMS-ish)
        float e = 0.0f;
        for (int i = chunkStart; i < chunkEnd; ++i)
        {
            float m = 0.5f * (scratchL[i] + scratchR[i]);
            e += m * m;
        }
        e = std::sqrt(e / (float)chunkLen); // RMS 0..~1

        // Map energy to hue: clamp to a reasonable range
        float energyNorm = juce::jlimit(0.0f, 1.0f, e * 3.0f); // tune multiplier
        float hue = juce::jmap(energyNorm, 0.0f, 1.0f, 0.60f, 0.00f); // blue->red
        float sat = juce::jmap(stereoWidth, 0.0f, 1.0f, 0.25f, 1.0f); // mono less saturated, stereo more
        float val = 1.0f;

        // Hue calculation - either energy-based or frequency-based
        if (fftMode)
        {
            // FFT MODE: Color based on frequency content
            float bass = processor.bassEnergy.load();
            float mid = processor.midEnergy.load();
            float high = processor.highEnergy.load();

            // Map frequencies to hue ranges
            // Bass = red/orange (0.0-0.1), Mids = green/yellow (0.3-0.4), Highs = blue/cyan (0.5-0.65)
            float dominantFreq = std::max({ bass, mid, high });

            if (bass == dominantFreq)
                hue = juce::jmap(bass, 0.0f, 1.0f, 0.0f, 0.1f); // Red-orange for bass
            else if (mid == dominantFreq)
                hue = juce::jmap(mid, 0.0f, 1.0f, 0.25f, 0.4f); // Green-yellow for mids
            else
                hue = juce::jmap(high, 0.0f, 1.0f, 0.5f, 0.65f); // Cyan-blue for highs

            hue = std::fmod(hue + hueControl + 1.0f, 1.0f);

            if (invertColors)
               hue = std::fmod(1.0f - hue + 1.0f, 1.0f);
                
        }
        else
        {
            // ENERGY MODE: Color based on overall energy (original behavior)
            hue = juce::jmap(energyNorm, 0.0f, 1.0f, 0.75f, 0.05f);
            hue = std::fmod(hue + hueControl + 1.0f, 1.0f);

            if (invertColors)
                hue = std::fmod(1.0f - hue + 1.0f, 1.0f);
               
        }

        // Saturation: controlled by user, modulated by stereo width
        float baseSat = juce::jmap(stereoWidth, 0.0f, 1.0f, 0.55f, 1.00f);
        sat = baseSat * satControl;
        sat = juce::jlimit(0.0f, 1.0f, sat);

        // Value (brightness): loud = brighter
        val = juce::jmap(energyNorm, 0.0f, 1.0f, 0.75f, 1.00f);

        // Apply colour
        g.setColour(juce::Colour::fromHSV(hue, sat, val, 1.0f));


        // Map to thickness: mono=thick, stereo=thin
        float thickness = juce::jmap(stereoWidth, 0.0f, 1.0f, 3.5f, 1.0f);
        thickness *= thicknessControl;  // Apply user control

        // Calculate spread multiplier: mono gets high multiplier, stereo gets 1.0
        float spreadMult = juce::jmap(stereoWidth, 0.0f, 1.0f, 20.0f, 1.0f);

        // Calculate waveform contribution: high for mono, zero for stereo
        float waveformAmount = juce::jmap(stereoWidth, 0.0f, 0.2f, 0.8f, 0.0f);
        waveformAmount = juce::jlimit(0.0f, 1.0f, waveformAmount);

        
        // Transform points for this chunk with dynamic spread
        for (int i = chunkStart; i < chunkEnd; ++i)
        {
            // Calculate mid and side with oscillating DC offset for wavy effect
            float offsetAmount = std::sin(dcPhase) * dcOffset;  // Oscillating offset
            float offsetL = scratchL[i] + offsetAmount;
            float offsetR = scratchR[i] - offsetAmount;
            float mid = (offsetL + offsetR) * 0.5f;
            float side = (offsetL - offsetR) * 0.5f;

            // Increment phase slowly for wave effect
            dcPhase += 0.001f * std::abs(dcOffset);  // Speed based on offset amount

            // Apply mono amount - reduce side signal
            side *= (1.0f - monoAmount);

            // Apply spread multiplier to side signal
            side *= spreadMult;

            // For mono content, create circular pattern
            // For mono content, create pattern based on selected shape
            const float monoWraps = processor.monoWrapsParam ? processor.monoWrapsParam->load() : 3.0f;
            float angle = ((float)i / (float)got) * juce::MathConstants<float>::twoPi * monoWraps;
            float radius = mid * 0.4f * gain * zoom * visualGainSmoothed;

            float patternX = 0.0f;
            float patternY = 0.0f;

            switch (monoShape)
            {
            case 0: // Circle
            {
                float r = radius; // No modulation for clean circle
                if (waveType > 0)
                {
                    float phase = std::fmod(angle / juce::MathConstants<float>::twoPi, 1.0f);
                    float modulation = getWaveModulation(phase);
                    r = radius * (1.0f + 0.3f * modulation);
                }
                patternX = std::cos(angle) * r;
                patternY = std::sin(angle) * r;
                break;
            }

            case 1: // Star (5 points)
            {
                float starAngle = angle;
                float phase = std::fmod(starAngle / juce::MathConstants<float>::twoPi, 0.2f) / 0.2f; // 5 cycles per rotation
                float modulation = getWaveModulation(phase);
                float r = radius * (1.0f + 2.0f * modulation);
                patternX = std::cos(starAngle) * r;
                patternY = std::sin(starAngle) * r;
                break;
            }

            case 2: // Square
            {
                float t = std::fmod(angle / (juce::MathConstants<float>::twoPi), 1.0f);
                float baseRadius = radius;

                // Add wave modulation
                if (waveType > 0)
                {
                    float phase = std::fmod(angle / juce::MathConstants<float>::twoPi, 1.0f);
                    float modulation = getWaveModulation(phase);
                    baseRadius = radius * (1.0f + 0.4f * modulation);
                }

                if (t < 0.25f)
                {
                    patternX = baseRadius;
                    patternY = juce::jmap(t, 0.0f, 0.25f, -baseRadius, baseRadius);
                }
                else if (t < 0.5f)
                {
                    patternX = juce::jmap(t, 0.25f, 0.5f, baseRadius, -baseRadius);
                    patternY = baseRadius;
                }
                else if (t < 0.75f)
                {
                    patternX = -baseRadius;
                    patternY = juce::jmap(t, 0.5f, 0.75f, baseRadius, -baseRadius);
                }
                else
                {
                    patternX = juce::jmap(t, 0.75f, 1.0f, -baseRadius, baseRadius);
                    patternY = -baseRadius;
                }
                break;
            }

            case 3: // Spiral
            {
                float spiralRadius = radius * (1.0f + angle / (juce::MathConstants<float>::twoPi * monoWraps) * 0.5f);

                // Add wave modulation to spiral
                if (waveType > 0)
                {
                    float phase = std::fmod(angle / juce::MathConstants<float>::twoPi, 1.0f);
                    float modulation = getWaveModulation(phase);
                    spiralRadius *= (1.0f + 0.3f * modulation);
                }

                patternX = std::cos(angle) * spiralRadius;
                patternY = std::sin(angle) * spiralRadius;
                break;
            }

            default: // Fallback to circle
                patternX = std::cos(angle) * radius;
                patternY = std::sin(angle) * radius;
                break;
            }

            // Stereo XY component
            float stereoX = (mid + side) * gain * zoom * visualGainSmoothed;
            float stereoY = (mid - side) * gain * zoom * visualGainSmoothed;

            // Blend between stereo XY and mono pattern
            float x = stereoX * (1.0f - waveformAmount) + patternX * waveformAmount;
            float y = stereoY * (1.0f - waveformAmount) + patternY * waveformAmount;

            // Apply rotation
            float xr = x * c - y * s;
            float yr = x * s + y * c;
            points[i] = { cx + xr * scale, cy - yr * scale };
        }

        if (particleMode)
        {
            // PARTICLE RENDERING MODE
            for (int i = chunkStart; i < chunkEnd; i += 4)
            {
                float progress = (float)(i - chunkStart) / (float)chunkLen;
                float segmentHue = std::fmod(hue + progress * 0.3f, 1.0f);

                // Particle size based on amplitude
                float particleSize = thickness * 2.0f;

                // Draw glow for particle
                if (glowIntensity > 0.0f)
                {
                    for (int glowPass = 0; glowPass < 3; ++glowPass)
                    {
                        float glowMult = glowSize - (glowPass * glowSize * 0.3f);
                        float glowAlpha = (0.15f / (glowPass + 1)) * glowIntensity;

                        // Glow stays saturated (progressively less saturated each layer for smooth gradient)
                        float glowSat = juce::jmap((float)glowPass, 0.0f, 2.0f, 1.0f, 0.7f); // Outer layers slightly less saturated
                        g.setColour(juce::Colour::fromHSV(segmentHue, glowSat, val, glowAlpha));
                        g.fillEllipse(points[i].x - (particleSize * glowMult) / 2,
                            points[i].y - (particleSize * glowMult) / 2,
                            particleSize * glowMult,
                            particleSize * glowMult);
                    }
                }

                // Draw core particle
                g.setColour(juce::Colour::fromHSV(segmentHue, sat, val, 1.0f));
                g.fillEllipse(points[i].x - particleSize / 2,
                    points[i].y - particleSize / 2,
                    particleSize,
                    particleSize);
            }
        }
        else
        {
            // LINE RENDERING MODE
// Multi-layer glow
            for (int glowPass = 0; glowPass < 3; ++glowPass)
            {
                float glowMult = glowSize - (glowPass * glowSize * 0.3f);
                float glowAlpha = (0.15f / (glowPass + 1)) * glowIntensity;

                for (int i = chunkStart; i < chunkEnd - 1; ++i)
                {
                    float progress = (float)(i - chunkStart) / (float)chunkLen;
                    float segmentHue = std::fmod(hue + progress * 0.3f, 1.0f);

                    // Glow stays saturated (progressively less saturated each layer)
                    float glowSat = juce::jmap((float)glowPass, 0.0f, 2.0f, 1.0f, 0.7f);
                    g.setColour(juce::Colour::fromHSV(segmentHue, glowSat, val, glowAlpha));

                    juce::Line<float> line(points[i], points[i + 1]);
                    g.drawLine(line, thickness * glowMult);  // This is the key - thick glow lines
                }
            }

            // Core pass: solid line on top (desaturates with saturation control)
            for (int i = chunkStart; i < chunkEnd - 1; ++i)
            {
                float progress = (float)(i - chunkStart) / (float)chunkLen;
                float segmentHue = std::fmod(hue + progress * 0.3f, 1.0f);

                // Core uses user saturation control (can go to white)
                g.setColour(juce::Colour::fromHSV(segmentHue, sat, val, 1.0f));

                juce::Line<float> line(points[i], points[i + 1]);
                g.drawLine(line, thickness);
            }

            // Core pass: solid line on top
            for (int i = chunkStart; i < chunkEnd - 1; ++i)
            {
                float progress = (float)(i - chunkStart) / (float)chunkLen;
                float segmentHue = std::fmod(hue + progress * 0.3f, 1.0f);

                g.setColour(juce::Colour::fromHSV(segmentHue, sat, val, 1.0f));

                juce::Line<float> line(points[i], points[i + 1]);
                g.drawLine(line, thickness);
            }
        }
    }
}
//==============================================================================
XYscopeAudioProcessorEditor::XYscopeAudioProcessorEditor(XYscopeAudioProcessor& p)
    : AudioProcessorEditor(&p),
    processor(p)
{
    setSize(600, 600);

    setResizable(true, true);
    setResizeLimits(300, 300, 2400, 2400); // pick sensible max for your display

    startTimerHz(60);

}



XYscopeAudioProcessorEditor::~XYscopeAudioProcessorEditor()
{
}

//==============================================================================
void XYscopeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (accumulation.isValid())
        g.drawImageAt(accumulation, 0, 0);
}


void XYscopeAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
