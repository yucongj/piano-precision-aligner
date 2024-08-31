
Piano Precision - Piano Aligner
================

### This is a Vamp plugin that works together with the host application [*Piano Precision*](https://github.com/yucongj/piano-precision), and conducts audio-to-score alignment.

The program reads in a digital score and a performance audio recording, and aligns these two mediums by finding the most plausible onset time stamp in the recording for each note in the score. The alignment method is based on a classic hidden Markov model from [Raphael](https://ieeexplore.ieee.org/document/761266).


## To Build and Install on Mac
To build from source code, run `make -f Makefile.osx`.


To install the package on Mac, copy `score-aligner.dylib` to the directory `$HOME/Library/Audio/Plug-Ins/Vamp/AudioToScoreAlignment/` (please create the folder `AudioToScoreAlignment` if it doesn't already exist).