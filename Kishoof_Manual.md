# Kishoof Manual

Controls
--------

![Panel](Graphics/Panel_Annotated.png?raw=true)

**1. LCD Screen** displays name of selected wavetable, waveforms, wavetable and warp positions and warp type

**2. Encoder** selects wavetables; press to change display from both to either wavetable

**3. Transpose**: 3 position control allowing octave up, down or normal for both channels

**4. Playback Mode** Up position plays a smoothly interpolated wavetable in channel A and a preset wavetable in channel B

**5. Channel A Wavetable** Wavetable frames are selected by a combination of the knob and CV via attenuator

**6. Channel B Wavetable** Wavetable frames are selected by a combination of the knob and CV

**7. Channel B Modulation** Modulate channel B output: Up position mixes in channel A; down position multiplies with Channel A (ring modulation)

**8. Channel B Octave** When button is pressed (LED on) channel B is one octave down from channel A

**9. Warp Type** Selects channel A warp type (Squeeze, Bend, Mirror, Through Zero FM). Press knob to reverse direction of channel B wavetable

**10. Warp Amount** Select the amount of modulation via knob and attenuatable CV

**11. USB C device** Used to access the internal storage as a USB drive; also provides a serial console for advanced utilities and configuration

**12. VCA input** Attach an envelope CV to control output volume of module

**13. Pitch** Attach a 1 volt per octave CV to control pitch over 6 octaves

Wavetables and Playback Mode
----------------------------

Kishoof plays back wavetables from an internal 64MB flash storage system. By connecting the module to a host via USB C the internal storage can be accessed and wavetables stored.

Supported Wavetable formats:
- wav file format
- Either 32 bit floating point or 16 bit fixed point
- Mono
- 2048 samples per frame
- Up to 256 frames

Wavetables are selected with the rotary encoder. If wavetables are stored in sub-folders these will be shown in yellow text and can be accessed by pressing the encoder button. If a wavetable is corrupt or in an unsupported format an error is shown on attempting to load:

- Fragged: Wavetable is fragmented in the file system
- Corrupt: The wav header cannot be read
- Format: Not 32 bit floating point or 16 bit integer
- Channels: Not mono
- Empty: No file data

### Smooth Playback Mode

With the playback mode switch in the up position, Channel A will playback the selected wavetable smoothly interpolating between the frames as the wavetable position is moved.

In this mode channel B plays a special internal wavetable which is user configurable via the console. This can contain up to 10 different wavetabhle frames chosen from: Sine at 1, 2, 3, 4, 5 or 6 times base frequency, Square, Saw, Triangle waves.

### Stepped Playback Mode

With the playback switch in the down position, wavetable frames are selected individually and not interpolated. In this mode both channel A and B can play any frame from the same wavetable.


Warping and Modulation
----------------------

### Channel A Modulation

The Warp Type knob selects the type of modulation applied to channel A:

- **Squeeze** - Warp amount to the left stretches the wave out from the center; to the right squeezes it into the middle

- **Bend** - Progressively squashes the wave from the right to the left

- **Mirror** - The first half of the wave cycle plays the wave forwards, the second half backwards. The Warp amount stretches the wave progressively from the outside to the inside

- **TZFM** - Applies through zero frequency modulation from channel B to channel A: as channel B's wave increases channel A's wavetable is played faster and vice versa. The overall amount of frequency modulation is proportional the base pitch so this will not produce enharmonic tones (as with standard FM).

### Channel B Modulation

The channel B modulation switch affects channel B's output: In the up position channel A's signal is added/mixed  and in the down position the two signals are multiplied (ie ring modulated).

Channel B can also be reversed by clicking the Warp Type button - orange arrows are shown at the bottom of the display to indicate if this is on.

Channel B can also be switched an octave down relative to channel A. This will affect channel A in TZFM mode, unlike the add/multiple modulations.


Calibration, Configuration and the Serial Console
-------------------------------------------------

When connecting a USB C cable to the module a serial console is available for calibration, configuration and trouble-shooting.

Use an appropriate serial console application (Termite, Putty etc) to connect. Type 'help' for a list of commands and 'info' for module information including firmware build date, calibration settings etc:

![Panel](Graphics/console.png?raw=true)

### Calibration

Calibration is used to configure the appropriate scaling for the 1 volt per octave signal. It also sets the normalisation level of the VCA control so the output is at full level with no envelope connected.

To calibrate it is necessary to generate a 0V and 1V signal which represent the lowest two C octaves the module can output. The calibration process will show its readings of the applied signals so try different octaves if the readings look out.

Type 'calib' to begin, check the measured values against the expected values and type 'y' to continue or 'x' to abort:

![Panel](Graphics/calib.png?raw=true)
