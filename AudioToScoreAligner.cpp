/*
  Yucong Jiang, June 2021
*/


#include "AudioToScoreAligner.h"
#include "Templates.h"
#include "Paths.h"
#include "SimpleHMM.h"

#include <cmath>
#include <filesystem>
#include <vector>

using namespace std;

AudioToScoreAligner::AudioToScoreAligner(float inputSampleRate, int hopSize,
int startEvent, int endEvent, int startFrame, int endFrame) :
     m_inputSampleRate{inputSampleRate} , m_hopSize{hopSize}, m_startEvent{startEvent}, 
     m_endEvent{endEvent}, m_startFrame{startFrame}, m_endFrame{endFrame}
{
}

AudioToScoreAligner::~AudioToScoreAligner()
{
}

bool AudioToScoreAligner::loadAScore(string scoreName, int blockSize)
{
    cerr << "In loadAScore: scoreName is -> " << scoreName << '\n';

    auto scores = Paths::getScores();

    if (scores.find(scoreName) == scores.end()) {
        cerr << "Score not found: " << scoreName << '\n';
        return false;
    }

    filesystem::path targetPath = scores[scoreName];

    // Paths::getScores() has already verified that these exist
    string scorePath = targetPath.string() + "/" + scoreName + ".solo";
    string scoreMeterPath = targetPath.string() + "/" + scoreName + ".meter";

    bool success = m_score.initialize(scorePath);
    if (success)    success = m_score.readMeter(scoreMeterPath);

    NoteTemplates t =
        CreateNoteTemplates::getNoteTemplates(m_inputSampleRate, blockSize);
    m_score.setEventTemplates(t);

    return success;
}

void AudioToScoreAligner::supplyFeature(DataSpectrum s)
{
    m_dataFeatures.push_back(s);
}

void AudioToScoreAligner::initializeLikelihoods()
{
    int frames = m_dataFeatures.size();
    int events = m_score.getMusicalEvents().size();
    if (frames == 0) {
        cerr << "AudioToScoreAligner::initializeLikelihoods:\
        features are not supplied." << '\n';
    }
    cerr << "AudioToScoreAligner::initializeLikelihoods:\
    features are indeed supplied. Number of frames = " << frames << '\n';
    for (int frame = 0; frame < frames; frame++) {
        vector<Likelihood> l;
        for (int event = 0; event < events; event++) {
            l.push_back(Likelihood(0, false));
        }
        m_likelihoods.push_back(l);
    }

    // event = -1 and -2:
    for (int frame = 0; frame < frames; frame++) {
        vector<Likelihood> l;
        l.push_back(Likelihood(0, false)); // event = -1
        l.push_back(Likelihood(0, false)); // event = -2
        m_silenceLikelihoods.push_back(l);
    }
}


double AudioToScoreAligner::getLikelihood(int frame, int event)
{
    if (m_dataFeatures.size() == 0) {
        cerr << "AudioToScoreAligner::getLikelihood:\
        features are not supplied." << '\n';
    }
    static Template silenceTemplate;
    static bool haveInitializedSilenceTemplates = false;
    if (!haveInitializedSilenceTemplates) { // TODO: Change it later.
        int bins = m_dataFeatures[frame].size();
        double low_freq = 20.;
        double low_proportion = .5;
        double p1 = low_proportion / low_freq;
        double p2 = (1 - low_proportion) / (double)bins;
        for (int bin = 0; bin < bins; bin++) {
            if (bin < low_freq) {
                silenceTemplate.push_back(p1 + p2);
            } else {
                silenceTemplate.push_back(p2);
            }
        }
        haveInitializedSilenceTemplates = true;
    }

    // TODO: check the range for frame and event
    // If event < 0, use a different template:
    if (event < 0) {
        if(!m_silenceLikelihoods[frame][abs(event)-1].calculated) {
            double score = 0;
            for (int bin = 0; bin < int(m_dataFeatures[frame].size()); bin++) {
                score += m_dataFeatures[frame][bin]*log(silenceTemplate[bin]);
                /*
                if (frame == 2 && event == -1) {
                    if (isnan(score)) {
                        cout << "bin="<<bin<<", score="<<score << '\n';
                        cout << "bin value: "<<m_dataFeatures[frame][bin] << '\n';
                        cout << "template value: "<<silenceTemplate[bin] << '\n';
                    }
                }
                */
            }
            m_silenceLikelihoods[frame][abs(event)-1].likelihood = exp(score);
            m_silenceLikelihoods[frame][abs(event)-1].calculated = true;
            /*
            if (isnan(m_silenceLikelihoods[frame][abs(event)-1].likelihood)) {
                cerr << "Frame="<<frame << ", event="<<event<<'\n';
                cerr << "like="<<m_silenceLikelihoods[frame][abs(event)-1].likelihood << '\n';
            }
            */
        }
        return m_silenceLikelihoods[frame][abs(event)-1].likelihood;
    }

    if (!m_likelihoods[frame][event].calculated) {
        const Score::MusicalEventList& eventList = m_score.getMusicalEvents();
        double score = 0;

        for (int bin = 0; bin < int(m_dataFeatures[frame].size()); bin++) {
            score += m_dataFeatures[frame][bin]*log(eventList[event].eventTemplate[bin]);
        }
        m_likelihoods[frame][event].likelihood = exp(score);
        m_likelihoods[frame][event].calculated = true;
    }

    return m_likelihoods[frame][event].likelihood;
}

AudioToScoreAligner::AlignmentResults AudioToScoreAligner::align()
{
    initializeLikelihoods(); // all zeros
    AlignmentResults results;

    SimpleHMM hmm = SimpleHMM(*this); // build state graph
    results = hmm.getAlignmentResults();

    return results;

/*
    // Maximum Likelihood Method:
    int frame = 0;
    for (const auto& feature: m_dataFeatures) {
        int maxEvent = 0;
        double maxL = 0;
        for (int event = 0; event < m_score.getMusicalEvents().size(); event++) {
            double l = getLikelihood(frame, event);
            if (l > maxL) {
                maxL = l;
                maxEvent = event;
            }
        }
        results.push_back(maxEvent);
        frame++;
    }

    return results;
*/
}

float AudioToScoreAligner::getSampleRate() const
{
    return m_inputSampleRate;
}

float AudioToScoreAligner::getHopSize() const
{
    return m_hopSize;
}

Score AudioToScoreAligner::getScore() const
{
    return m_score;
}

AudioToScoreAligner::DataFeatures AudioToScoreAligner::getDataFeatures() const
{
    return m_dataFeatures;
}

void AudioToScoreAligner::setAlignmentConstraints(int se, int ee, int sf, int ef)
{
    m_startEvent = se;
    m_endEvent = ee;
    m_startFrame = sf;
    m_endFrame = ef;
}

int AudioToScoreAligner::getStartEvent() const
{
    return m_startEvent;
}

int AudioToScoreAligner::getEndEvent() const
{
    return m_endEvent;
}

int AudioToScoreAligner::getStartFrame() const
{
    return m_startFrame;
}

int AudioToScoreAligner::getEndFrame() const
{
    return m_endFrame;
}
