/*
  modified by Yucong Jiang, June 2021
*/


// Remember to use a different guard symbol in each header!
#ifndef PIANO_ALIGNER_PLUGIN_H
#define PIANO_ALIGNER_PLUGIN_H

#include <vamp-sdk/Plugin.h>

#include "AudioToScoreAligner.h"

using std::string;


class PianoAligner : public Vamp::Plugin
{
public:
    PianoAligner(float inputSampleRate);
    virtual ~PianoAligner();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    // plugin-specific data and methods go here
    AudioToScoreAligner *m_aligner;
    int m_blockSize;

    // Constraints for partial alignments. In each case a value of -1
    // indicates no constraint of that type. The defaults are all -1.
    float m_scorePositionStart;
    float m_scorePositionEnd;
    float m_audioStart_sec;
    float m_audioEnd_sec;
    
    bool m_isFirstFrame;
    Vamp::RealTime m_firstFrameTime;
    int m_frameCount;
    string m_scoreName;
};



#endif
