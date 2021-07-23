/*
  Yucong Jiang, July 2021
*/

#include "SimpleHMM.h"

#include <cmath>

static const int BEAM_SEARCH_WIDTH = 200;//200;

using Hypothesis = SimpleHMM::Hypothesis;
using State = SimpleHMM::State;

SimpleHMM::SimpleHMM(AudioToScoreAligner& aligner) : m_aligner{aligner}
{
    // Build the state graph.
    Score score = m_aligner.getScore();
    Score::MusicalEventList events = score.getMusicalEvents();
    float tempo = score.getDefaultTempo();
    int denom = score.getTimeSignatureDenom();
    float sr = m_aligner.getSampleRate();
    int hopSize = m_aligner.getHopSize();
    if (hopSize == 0) {
        std::cerr << "hopSize = 0 in SimpleHMM()." << '\n';
        return;
    }

    // specify the starting state
    double p = 0.975; // self-loop
    double tailProb  = 1 - p; // leaving the micro state
    m_startingState = new State(-1, 0);
    m_startingState->nextStates[m_startingState] = p;
    m_startingState->prevStates[m_startingState] = p;
    State* tail = m_startingState;
    //std::cout << "tail:" << State::toString(*tail) << '\n';


    // add micro states for each event
    int eventIndex = 0;
    for (auto& event : events) {
        double secs = event.duration.getValue() * denom * 60. / tempo;
        double frames = secs * sr / (double)hopSize;
        double var = (0.25*0.25) * frames * frames;
        int M = round(frames*frames / (var + frames));
        if (M < 1)  M = 1;
        p = 1. - M / frames; // frames shouldn't be 0
        //std::cout << "frames = "<<frames<<", var="<<var<<", M = " << M <<", p="<<p << '\n';
        for (int m = 0; m < M; m++) {
            // add a state
            State* newState = new State(eventIndex, m);
            newState->nextStates[newState] = p; // self-loop
            newState->prevStates[newState] = p; // self-loop
            if (m == 0) {
                tail->nextStates[newState] = tailProb;
                newState->prevStates[tail] = tailProb;
            } else {
                tail->nextStates[newState] = 1-p; // leave state
                newState->prevStates[tail] = 1-p;
            }
            tail = newState;
        }
        tailProb = 1 - p;
        eventIndex++;
    }

    // add the ending state
    State* lastState = new State(-2, 0);
    lastState->nextStates[lastState] = 1.;
    lastState->prevStates[lastState] = 1.;
    tail->nextStates[lastState] = tailProb;
    lastState->prevStates[tail] = tailProb;

    // test:
/*
    State* current = m_startingState;
    while (current->nextStates.size() > 0) {
        std::cout << "event/microIndex= " << current->eventIndex <<"/"<< current->microIndex<< '\n';
        for (auto& p : current->nextStates) {
            //std::cout << p.first->eventIndex <<"/"<< p.first->microIndex<< "\t" << p.second << '\n';
            std::cout << State::toString(*p.first) << '\n';
        }
        if (current->nextStates.size() < 2) break;
        for (auto& q: current->nextStates) {
            if (q.first != current) {
                current = q.first;
                break;
            }
        }

    }
*/
}

SimpleHMM::~SimpleHMM()
{
    // TODO: Delete each state in the state graph (nextStates).
    // YJ: Is there a better way?
}

static void getForwardProbs(vector<vector<Hypothesis>>* forward,
    AudioToScoreAligner& aligner,
    const SimpleHMM::StateGraph* startingState) {

        int totalFrames = aligner.getDataFeatures().size();
        forward->reserve(totalFrames);
        vector<Hypothesis> hypotheses;
        // first frame:
        hypotheses.push_back(Hypothesis(startingState, 1.));
        forward->push_back(hypotheses);

        // later frames:
        for (int frame = 1; frame < totalFrames; frame++) {
            hypotheses.clear();
            for (const auto& hypo : forward->at(frame-1)) {
                double prior = hypo.prob;
                for (const auto& next : hypo.state->nextStates) {
                    double trans = next.second;
                    int event = next.first->eventIndex;
                    double like;
                    if (event < 0)  like = 0.0001; // TODO: change to a real likelihood
                    else    like = aligner.getLikelihood(frame, event);
                    hypotheses.push_back(Hypothesis(next.first, prior*trans*like));
                }
            }
            // Merge, sort (and trim), and then normalize.
            unordered_map<const State*, double> merged;
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
            std::sort(hypotheses.begin(), hypotheses.end(), std::greater<Hypothesis>());
            if (hypotheses.size() > BEAM_SEARCH_WIDTH)
                hypotheses.erase(hypotheses.begin() + BEAM_SEARCH_WIDTH, hypotheses.end());
            double total = 0.;
            for (const auto& h : hypotheses) {
                total += h.prob;
            }
            // Check total != 0;
            if (total == 0) std::cout << "Total is zero!" << '\n';
            for (auto& h : hypotheses) {
                h.prob /= total;
            }
            forward->push_back(hypotheses);
/*
            std::cout << "Frame = " << frame << '\n';
            for (auto& h : forward->at(frame)) {
                std::cout << Hypothesis::toString(h) << '\n';
            }
*/
        }
}

AudioToScoreAligner::AlignmentResults SimpleHMM::getAlignmentResults()
{
    AudioToScoreAligner::AlignmentResults results;

    vector<vector<Hypothesis>>* forward = new vector<vector<Hypothesis>>();
    getForwardProbs(forward, m_aligner, m_startingState);
    vector<vector<Hypothesis>>* backward = new vector<vector<Hypothesis>>();
    //getBackwardProbs(backward, m_aligner, m_startingState);
    std::cout << forward << '\n';
    for (const auto& l : *forward) {
        map<int, double> merged;
        for (const auto& h : l) {
            if (merged.find(h.state->eventIndex) == merged.end()) {
                merged[h.state->eventIndex] = h.prob;
            } else {
                merged[h.state->eventIndex] += h.prob;
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
        results.push_back(record);
    }


    return results;
}
