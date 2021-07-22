/*
  Yucong Jiang, July 2021
*/

#include "SimpleHMM.h"

#include <cmath>

using std::make_pair;

static const int BEAM_SEARCH_WIDTH = 200;

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
    State* tail = &m_startingState;

    // specify the starting state
    //m_startingState.eventIndex = -1;
    //m_startingState.microIndex = 0;
    double p = 0.975; // self-loop
    double tailProb  = 1 - p; // leaving the micro state
    m_startingState.nextStates.push_back(make_pair(&m_startingState, p));
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
            //newState->eventIndex = eventIndex;
            //newState->microIndex = m;
            newState->nextStates.push_back(make_pair(newState, p)); // self-loop
            if (m == 0) {
                tail->nextStates.push_back(make_pair(newState, tailProb));
            } else {
                tail->nextStates.push_back(make_pair(newState, 1-p)); // leave state
            }
            tail = newState;
        }
        tailProb = 1 - p;
        eventIndex++;
    }

    // add the ending state
    State* lastState = new State(-2, 0);
    //lastState->eventIndex = -2;
    //lastState->microIndex = 0;
    lastState->nextStates.push_back(make_pair(lastState, 1.));
    tail->nextStates.push_back(make_pair(lastState, tailProb));

    // test:
    /*
    State *current = &m_startingState;
    while (current->nextStates.size() > 0) {
        std::cout << "event/microIndex= " << current->eventIndex <<"/"<< current->microIndex<< '\n';
        for (auto& p : current->nextStates) {
            std::cout << p.first->eventIndex <<"/"<< p.first->microIndex<< "\t" << p.second << '\n';
        }
        if (current->nextStates.size() < 2) break;
        current = current->nextStates[1].first;
    }
    */
}

SimpleHMM::~SimpleHMM()
{
    // TODO: Delete each state in the state graph (nextStates).
    // YJ: Is there a better way?
}

AudioToScoreAligner::AlignmentResults SimpleHMM::getAlignmentResults()
{
    AudioToScoreAligner::AlignmentResults results;
    vector<vector<Hypothesis>>* forward;
    getForwardProbs(forward, m_aligner, m_startingState);
    vector<vector<Hypothesis>>* backward;
    getBackwardProbs(backward, m_aligner, m_startingState);

    return results;
}
