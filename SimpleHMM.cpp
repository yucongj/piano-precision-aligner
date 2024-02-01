/*
  Yucong Jiang, July 2021
*/

#include "SimpleHMM.h"

#include <cmath>
#include <map>
#include <algorithm>

using namespace std;

static const int BEAM_SEARCH_WIDTH = 200;

using Hypothesis = SimpleHMM::Hypothesis;
using State = SimpleHMM::State;

SimpleHMM::SimpleHMM(AudioToScoreAligner& aligner) : m_aligner{aligner}
{
    // Build the state graph: m_nextStates and m_prevStates.
    Score score = m_aligner.getScore();
    Score::MusicalEventList events = score.getMusicalEvents();
    float sr = m_aligner.getSampleRate();
    int hopSize = m_aligner.getHopSize();
    if (hopSize == 0) {
        cerr << "hopSize = 0 in SimpleHMM()." << '\n';
        return;
    }

    // specify the starting state
    double p = 0.975; // self-loop
    double tailProb  = 1 - p; // leaving the micro state
    State startingState = State(-1, 0);
    m_nextStates[startingState][startingState] = p;
    m_prevStates[startingState][startingState] = p;
    State tail = startingState;
    // cerr << "tail:" << State::toString(tail) << '\n';

    // add micro states for each event
    int startEvent = m_aligner.getStartEvent();
    int endEvent = m_aligner.getEndEvent();
    for (int eventIndex = startEvent; eventIndex <= endEvent; eventIndex++) {
        auto &event = events[eventIndex];
        if (event.tempo == 0.0) {
            cerr << "In SimpleHMM: event.tempo is zero!!!" << '\n';
        }
        double secs = event.duration.getValue() * 4 * 60. / event.tempo; // tempo is defined in quarter note
        double frames = secs * sr / (double)hopSize;
        double var = (0.25*0.25) * frames * frames;
        int M = round(frames*frames / (var + frames));
        if (M < 1)  M = 1;
        p = 1. - M / frames; // frames shouldn't be 0
        //cout << "frames = "<<frames<<", var="<<var<<", M = " << M <<", p="<<p << '\n';
        for (int m = 0; m < M; m++) {
            // add a state
            State newState = State(eventIndex, m);
            m_nextStates[newState][newState] = p; // self-loop
            m_prevStates[newState][newState] = p; // self-loop
            if (m == 0) {
                m_nextStates[tail][newState] = tailProb;
                m_prevStates[newState][tail] = tailProb;
            } else {
                m_nextStates[tail][newState] = 1-p; // leave state
                m_prevStates[newState][tail]  = 1-p;
            }
            tail = newState;
        }
        tailProb = 1 - p;
    }

    // add the ending state
    State lastState = State(-2, 0);
    m_nextStates[lastState][lastState] = 1.;
    m_prevStates[lastState][lastState] = 1.;
    m_nextStates[tail][lastState] = tailProb;
    m_prevStates[lastState][tail] = tailProb;


    // test:
    /*
    State current = startingState;
    while (m_nextStates[startingState].size() > 0) {
        cerr << "event/microIndex= " << current.eventIndex <<"/"<< current.microIndex<< '\n';
        for (auto& p : m_nextStates[current]) {
            cerr << State::toString(p.first) << "\t"<<p.second<< '\n';
        }
        cerr << "Prev:" << '\n';
        for (auto& p : m_prevStates[current]) {
            cerr << State::toString(p.first) << "\t"<<p.second<< '\n';
        }
        if (m_nextStates[current].size() < 2) break;
        for (auto& q: m_nextStates[current]) {
            if (!(q.first == current)) {
                current = q.first;
                break;
            }
        }
    }
    */

}

SimpleHMM::~SimpleHMM()
{
}

/*
const map<State, map<State, double>>& SimpleHMM::getNextStates() const
{
    return m_nextStates;
}

const map<State, map<State, double>>& SimpleHMM::getPrevStates() const
{
    return m_prevStates;
}
*/

static void getForwardProbs(vector<vector<Hypothesis>>* forward,
    AudioToScoreAligner& aligner, const map<State, map<State, double>>& nextStates) {

        int startFrame = aligner.getStartFrame();
        int endFrame = aligner.getEndFrame();
        int totalFrames = endFrame + 1 - startFrame;
        cerr << "In getForwardProbs: totalFrames = " << totalFrames << '\n';
        // TODO: Assert totalFrames is at least 1
        forward->reserve(totalFrames);
        vector<Hypothesis> hypotheses;
        // first frame:
        hypotheses.push_back(Hypothesis(State(-1, 0), 1.));
        forward->push_back(hypotheses);

        // later frames:
        for (int frame = 1; frame < totalFrames; frame++) {
            hypotheses.clear();
            for (const auto& hypo : forward->at(frame-1)) {
                double prior = hypo.prob;
                for (const auto& next : nextStates.at(hypo.state)) {
                    double trans = next.second;
                    int event = next.first.eventIndex;
                    double like;
                    like = aligner.getLikelihood(startFrame + frame, event);
                    hypotheses.push_back(Hypothesis(next.first, prior*trans*like));
                }
            }
            // Merge, sort (and trim), and then normalize.
            map<State, double> merged;
            for (const auto& h : hypotheses) {
                if (merged.find(h.state) == merged.end()) {
                    merged[h.state] = h.prob;
                } else {
                    merged[h.state] += h.prob;
                }
            }
            hypotheses.clear();
            for (const auto& h: merged) {
                hypotheses.push_back(Hypothesis(h.first, h.second));
            }
            sort(hypotheses.begin(), hypotheses.end(), greater<Hypothesis>());
            if (hypotheses.size() > BEAM_SEARCH_WIDTH)
                hypotheses.erase(hypotheses.begin() + BEAM_SEARCH_WIDTH, hypotheses.end());
            double total = 0.;
            for (const auto& h : hypotheses) {
                total += h.prob;
            }
            if (total == 0) cerr << "In getForwardProbs: total is zero!!!" << '\n';
            for (auto& h : hypotheses) {
                h.prob /= total;
            }
            forward->push_back(hypotheses);
            /*
            cerr << "In getForwardProbs: frame = " << frame << '\n';
            for (auto& h : forward->at(frame)) {
                cerr << "new prior = "<<Hypothesis::toString(h) << '\t'<<"likelihood = " << aligner.getLikelihood(startFrame+frame, h.state.eventIndex) << '\n';
            }
            */
        }
}



static void getBackwardProbs(vector<vector<Hypothesis>>* backward,
    AudioToScoreAligner& aligner, const map<State, map<State, double>>& prevStates) {
        int startFrame = aligner.getStartFrame();
        int endFrame = aligner.getEndFrame();
        int totalFrames = endFrame + 1 - startFrame;
        cerr << "In getBackwardProbs: totalFrames = " << totalFrames << '\n';
        // TODO: Assert totalFrames is at least 1
        backward->resize(totalFrames);
        vector<Hypothesis> hypotheses;

        // last frame:
        hypotheses.push_back(Hypothesis(State(-2, 0), 1.));
        if (totalFrames > 0) {
            backward->at(totalFrames - 1) = hypotheses;
        }

        for (int frame = totalFrames - 2; frame >= 0; frame--) {
            hypotheses.clear();
            for (const auto& hypo : backward->at(frame + 1)) {
                double prior = hypo.prob;
                int event = hypo.state.eventIndex;
                double like;
                like = aligner.getLikelihood(startFrame + frame + 1, event);

                for (const auto& prev : prevStates.at(hypo.state)) {
                    double trans = prev.second;
                    hypotheses.push_back(Hypothesis(prev.first, prior*trans*like));
                }
            }
            // Merge, sort (and trim), and then normalize.
            map<State, double> merged;
            for (const auto& h : hypotheses) {
                if (merged.find(h.state) == merged.end()) {
                    merged[h.state] = h.prob;
                } else {
                    merged[h.state] += h.prob;
                }
            }
            hypotheses.clear();
            for (const auto& h: merged) {
                hypotheses.push_back(Hypothesis(h.first, h.second));
            }
            sort(hypotheses.begin(), hypotheses.end(), greater<Hypothesis>());
            if (hypotheses.size() > BEAM_SEARCH_WIDTH)
                hypotheses.erase(hypotheses.begin() + BEAM_SEARCH_WIDTH, hypotheses.end());
            double total = 0.;
            for (const auto& h : hypotheses) {
                total += h.prob;
            }
            if (total == 0) cerr << "In getBackwardProbs: total is zero!!!" << '\n';
            for (auto& h : hypotheses) {
                h.prob /= total;
            }
            backward->at(frame) = hypotheses;
            /*
            cerr << "In getBackwardProbs: startFrame + frame = " << startFrame + frame << '\n';
            for (auto& h : backward->at(frame)) {
                cerr << Hypothesis::toString(h) << '\n';
            }
            */

        }
}





AudioToScoreAligner::AlignmentResults SimpleHMM::getAlignmentResults()
{
    AudioToScoreAligner::AlignmentResults results;

    vector<vector<Hypothesis>>* forward = new vector<vector<Hypothesis>>();

    getForwardProbs(forward, m_aligner, m_nextStates);
    vector<vector<Hypothesis>>* backward = new vector<vector<Hypothesis>>();
    getBackwardProbs(backward, m_aligner, m_prevStates);
    vector<vector<Hypothesis>> post;
    vector<Hypothesis> hypotheses;
    int startFrame = m_aligner.getStartFrame();
    int endFrame = m_aligner.getEndFrame();
    for (int frame = startFrame; frame <= endFrame; frame ++) {
        hypotheses.clear();
        for (const auto& hypo1 : forward->at(frame-startFrame)) {
            for (const auto& hypo2 : backward->at(frame-startFrame)) {
                if (hypo1.state == hypo2.state) {
                    hypotheses.push_back(Hypothesis(hypo1.state, hypo1.prob * hypo2.prob));
                    break;
                }
            }
        }
        post.push_back(hypotheses);
    }

    // Print posterior hypotheses:
    /*
    int frame = startFrame;
    cerr << "Forward!!!" << '\n';
    for (auto& p : post) { // *forward
        sort(p.begin(), p.end(), greater<Hypothesis>());
        cerr << "### Frame = " << frame << '\n';
        cerr << "### real time = " <<Vamp::RealTime::frame2RealTime(frame*(128.*6.), 48000)<< '\n'; // m_firstFrameTime is 0 in SV
        for (const auto& h : p) {
            cerr << Hypothesis::toString(h) << '\n';
        }
        frame++;
    }
    */


    // Window
    int windowSize = 3; // TODO: Check and make sure it's always an odd number.
    cout << "windowSize/2 = "<<windowSize/2 << '\n';
    int startEvent = m_aligner.getStartEvent();
    int endEvent = m_aligner.getEndEvent();
    int onsetFrame = 0;
    for (int event = startEvent; event <= endEvent; event++) {
        if (results.size() == 0)    onsetFrame = 0;
        else onsetFrame = results[results.size()-1] - startFrame - windowSize/2 + 1;
        if (onsetFrame < 0) onsetFrame = 0;
        double bestScore = 0.;
        int bestOnsetFrame;
        for (int frame = onsetFrame; frame + windowSize < int(post.size()) + 1; frame++) {
            // find the best onsetFrame for this event, and add frame to result:
            double score = 0.;
            for (int t = frame; t < frame + windowSize; t++) {
                for (const auto& h: post[t]) {
                    if (h.state.eventIndex == event && h.state.microIndex == 0) {
                        score += h.prob;
                    }
                }
            }
            if (score > bestScore) {
                bestScore = score;
                bestOnsetFrame = frame + windowSize/2;
            }
        }
        results.push_back(startFrame + bestOnsetFrame);
        cerr << "Event="<<event<<", onsetFrame = " << startFrame + bestOnsetFrame << '\n';
    }

    return results;



    // Old: Return the the event with maximum posterior prob for each frame.
    /*
    frame = 0;
    for (const auto& l : post) { //forward
        map<int, double> merged;
        for (const auto& h : l) {
            if (merged.find(h.state.eventIndex) == merged.end()) {
                merged[h.state.eventIndex] = h.prob;
            } else {
                merged[h.state.eventIndex] += h.prob;
            }
        }
        double highest = 0.;
        int record = 0;
        for (const auto& p : merged) {
            if (p.second > highest) {
                highest = p.second;
                record = p.first;
            }
        }
        //cout << "Frame = "<<frame<< '\n';
        //cout << "record = "<<record<<", highest prob = " <<highest << '\n';
        frame++;
        // results.push_back(record);
    }
    return results;
    */
}
