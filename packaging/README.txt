Keyy @VERSION@ - key, tuning and tempo detection by tooL8
==========================================================

Load an audio file (or drop it onto the window) and Keyy tells you its
key and relative key, its tuning reference and its tempo.

Free software under the GNU AGPLv3 (see LICENSE.txt).
Source code: https://github.com/tool8audioDE/Keyy


WHAT'S IN THIS ZIP
------------------
  Keyy.vst3      the plugin (a folder - keep it as it is)
  Keyy.exe       standalone app, runs without a DAW
  LICENSE.txt    GNU AGPLv3
  README.txt     this file

Windows 10/11, 64-bit. VST3 only.


INSTALL THE PLUGIN
------------------
1. Copy the whole folder "Keyy.vst3" to
       C:\Program Files\Common Files\VST3
   (Windows will ask for administrator permission.)
2. FL Studio: Options -> Manage plugins -> Find more plugins.
   Other DAWs: rescan your VST3 plugins.
3. Put Keyy as an effect on any mixer track. Audio passes through
   unchanged.

The standalone app needs no installation - just start "Keyy.exe".
Windows SmartScreen may warn because the app is not code-signed:
click "More info" -> "Run anyway".


HOW TO USE
----------
Click "Load File..." or drag a file from the FL Studio browser or from
Explorer onto the window. Supported: WAV, AIFF, FLAC, MP3, Ogg.

  Key            e.g. "F Minor", with its relative key ("= Ab Major").
                 Both use the same seven notes, so for pitch correction
                 either one works. "Relative Key" swaps them.
  Tuning         e.g. "A = 437.0 Hz (-11.9 cents)" - useful for samples
                 from old records that are not tuned to 440 Hz.
  Tempo          in BPM, with /2 and x2 buttons. "from loop length" means
                 the file is an exact number of bars long, so the tempo
                 was calculated from its length.

The result is saved with your project and shows up again when you reopen
it, even if the audio file has moved.

Key detection is not always right: on the developer's own loops and beats
it finds the exact key about 6 times out of 10, and the right scale (key
or relative key) about 7 times out of 10. Trust your ears.


https://tool8.online
