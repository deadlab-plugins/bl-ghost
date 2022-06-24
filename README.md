# bl-ghost
BlueLab Ghost spectrogram viewer (plugin and app)

Ghost is a powerful spectrogram visualization and editing plugin.

Ghost provides a large set of features for visualizing and editing spectrograms:

* Powerful and intuitive zoom, with adaptive level of detail.
* Possibility to select any region of the spectrogram, and to play this region only.
* File opening and saving (.wav and .aif).
* Real time view mode, with scrolling spectrogram.
* A set of 4 colormaps.
* Brightness and contrast.
* Options to display the corresponding waveform in addition to the spectrogram.
* Choice between several interface sizes, small and medium...
* Interactive mini view of the overall sound file.
* Editing tools: cut.
* Editing tool: gain increase or decrease of the selected region.
* Undo mechanism.
* Files drag'n drop
* Tabs

It is usable on Linux, MacOS and Windows, as plugin or standalone application.

## Building on Linux
To build Ghost app, vst2 and vst3:
- launch ./build.sh
- at a moment, you may have to launch iPlug2/Dependencies/IPlug/download-iplug-sdks.sh

## Building on Mac
- go to iPlug2/Examples
- make a symbolic link to bl-ghost, to get: iPlug2/Examples/bl-ghost pointing to the main bl-ghost directory
- open bl-ghost.xcworkspace and build

## Building on Windows
- go to iPlug2/Examples
- make a shortcut to bl-ghost, to get: iPlug2/Examples/bl-ghost pointing to the main bl-ghost directory
- open bl-ghost.sln and build

To see it in action, have a look at [this video](https://www.youtube.com/watch?v=womul8SFWa4)

![bl-ghost snapshot](https://github.com/deadlab-plugins/bl-ghost/blob/master/Images/bl-ghost.png)
