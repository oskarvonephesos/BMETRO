# BMETRO

BMETRO is a tiny application you can use to create click tracks in various audio formats. It features compound time signatures, automatic BPM calculation with accel. and rit. and the ability to save both audio files and 'raw' text files. The app uses the NCURSES library as a GUI, so it is only available on UNIX based systems.

# INSTALLATION

To install, download, navigate to the directory and either run make or ./configure. Configure will attempt to install sox via homebrew and, if this succeeds, will enable exporting to mp3 and aif which reduces file sizes by a factor of 20.

# LICENSE INFORMATION

BMETRO uses the libwav library available here on GitHub which is licensed under the Mozilla Public License 2.0 (which is included in the appropriate file). BMETRO itself is licensed under the GNU GENERAL PUBLIC LICENSE.
