# Platformer2D/Audio Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/Audio/`.
- OPL emulation, PC speaker synthesis, mixing, timing, U++ audio output adapter.

## Responsibilities

1. **OPL Emulation**
   - DBOPL (DOSBox) and Nuked OPL3 support
   - Register write emulation (0x20-0xFF)
   - 9 channels (OPL2) / 18 channels (OPL3)
   - 4-operator percussion mode
   - Waveform selection, ADSR envelopes, tremolo/vibrato

2. **PC Speaker Synthesis**
   - 1-bit PWM output
   - Frequency generation via PIT channel 2
   - Mixing with OPL output

3. **Audio Mixing**
   - Stereo output (OPL is mono, mixed to stereo)
   - Volume control per channel
   - Master volume
   - Sample rate conversion (44.1/48 kHz)

4. **IMF Music Playback**
   - AdLib IMF format (from AUDIO chunks)
   - Tempo control (70 Hz tick based)
   - Loop handling

5. **Digitized Sound Effects**
   - Huffman-decoded PCM from AUDIO
   - PC speaker / Sound Blaster formats
   - Priority-based mixing (max simultaneous sounds)

6. **Timing**
   - PIT-derived 70/140 Hz timer
   - Audio callback synchronization option
   - Deterministic tick generation for headless mode

7. **U++ Audio Output Adapter**
   - `Sound` package integration
   - Callback or push-based output
   - Null output for headless testing

## Key Types

- `OPLEmulator` - Abstract base for DBOPL/NukedOPL3
- `PCSpeaker` - 1-bit synthesis
- `AudioMixer` - Combines OPL + PC speaker + SFX
- `IMFPlayer` - Music sequence playback
- `SD_Backend` interface adapted to U++ Sound

## Binary Compatibility

- OPL register behavior matches OmniSpeak `id_sd_opl2.c` / `nuked_opl3.c`
- IMF parsing matches original format
- Sound effect priorities and mixing match original

## Dependencies

- Uses: `Platformer2D`, `Core` (for timing), `Resource` (for audio chunks)
- Used by: `Platform`, `Platformer2DTest`

## U++ Sound Integration

- Platform layer creates `Sound` output stream
- Audio callback pulls mixed samples from `AudioMixer`
- Headless mode uses null callback