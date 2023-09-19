
Piano Precision - Piano Aligner
================

### This program works together with [Piano Precision] (https://github.com/yucongj/piano-precision), and conducts audio-to-score alignment.

The program reads in a digital score and a performance audio recording, and aligns these two mediums by finding the most plausible onset time stamp in the recording for each note in the score. The alignment method is based on a hidden Markov model.

Project page: TBA.

## To Build and Install on Mac
To build from source code, navigate to the folder on Terminal, and run `make -f Makefile.osx`


To install the package on Mac, copy `score-aligner.dylib`, `score-aligner.cat`, and `score-aligner.n3` to the folder `$HOME/Library/Audio/Plug-Ins/Vamp`.
