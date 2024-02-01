/*
  Yucong Jiang, June 2021
*/

#ifndef AUDIO_TO_SCORE_ALIGNER_H
#define AUDIO_TO_SCORE_ALIGNER_H


#include "Score.h"
#include "vamp-sdk/Plugin.h"

#include <vector>


class AudioToScoreAligner
{
public:
    AudioToScoreAligner(float inputSampleRate, int hopSize,
     int m_startEvent, int m_endEvent, int m_startFrame, int m_endFrame);
    ~AudioToScoreAligner();

/*
    struct AlignmentResults
    {
        std::map<Score::MusicalEvent, Vamp::RealTime> alignments;
    };
*/
    struct Likelihood {
        double likelihood;
        bool calculated;
        Likelihood(double l, bool c) : likelihood{l}, calculated{c} { }
    };
    typedef std::vector<std::vector<Likelihood>> DataLikelihoods;
    typedef std::vector<float> DataSpectrum;
    typedef std::vector<DataSpectrum> DataFeatures;

    //typedef std::vector<Vamp::RealTime> AlignmentResults;
    typedef std::vector<int> AlignmentResults;

    bool loadAScore(std::string scoreName, int blockSize);
    void supplyFeature(DataSpectrum s);
    AlignmentResults align();
    float getSampleRate() const;
    float getHopSize() const;
    Score getScore() const;
    DataFeatures getDataFeatures() const;
    double getLikelihood(int frameIndex, int eventIndex);
    void setAlignmentConstraints(int se, int ee, int sf, int ef);
    int getStartEvent() const;
    int getEndEvent() const;
    int getStartFrame() const;
    int getEndFrame() const;

private:
    float m_inputSampleRate;
    int m_hopSize;
    Score m_score;
    DataLikelihoods m_likelihoods;
    DataLikelihoods m_silenceLikelihoods;
    DataFeatures m_dataFeatures;
    int m_startEvent;
    int m_endEvent;
    int m_startFrame;
    int m_endFrame;

    void initializeLikelihoods();
};

#endif
