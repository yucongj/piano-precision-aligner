/*
  Yucong Jiang, July 2021
*/

#include "Templates.h"

#include <cmath>
#include <iostream>
#include <map>
#include <vector>



static const int LOW_MIDI = 21;
static const int HIGH_MIDI = 108;
static const int MAX_HARMONICS_COUNT = 16;

static float peakFunction(float x) { // MAY CHANGE THIS FUNCTION LATER
    return exp(-0.5*x*x);
}

static float midiToFreq(int midi) {
    return pow(2., (midi-69)/12.)*440.;
}

static void initializeNoteTemplates(float sr, int blockSize, NoteTemplates& t) { // Is "static" unnecessary here?
    int scale = 6; // This is hard-coded for now; needs to be changed later.
    int bins = (blockSize/2)/scale; // no DC
    int N = blockSize;

    for (int midi = LOW_MIDI; midi <= HIGH_MIDI; midi++) {
        float f0 = midiToFreq(midi);
        t[midi].resize(bins, 0);

        int h = 1; // first harmonic
        int bin = round(1*f0*N/(double)sr) - 1; // The "-1" may not matter.
        if (bin < 0)    bin = 0;
        while (bin < bins && h <= MAX_HARMONICS_COUNT) {
            float sigma = bin * 0.01 + 0.01; // Need to test these constants
            for (int k = 0; k < bins; k++) {
                float x = (k-bin)/sigma;
                t[midi][k] += exp(-k*0.05)*peakFunction(x); // 0.01//0.1
            }
            h++;
            bin = round(h*f0*N/(double)sr); // hth harmonic
        }
        //Normalize:
        float total = 0;
        for (const auto &value: t[midi]) {
            total += value;
        }
        // TODO: check total != 0.
        if (total == 0.) {
            std::cerr << "In initializeNoteTemplates: total is 0." << '\n';
        } else {
            for (auto &value: t[midi]) {
                value /= total;
            }
        }
    }
}

const NoteTemplates&
CreateNoteTemplates::getNoteTemplates(float sampleRate, int blockSize)
{
    static NoteTemplates t;
    static bool haveInitializedTemplates = false;
    if (!haveInitializedTemplates) {
        initializeNoteTemplates(sampleRate, blockSize, t);
        haveInitializedTemplates = true;
    }
    return t;
}
