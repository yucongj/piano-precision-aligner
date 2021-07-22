/*
  Yucong Jiang, July 2021
*/

#ifndef SIMPLE_HMM_H
#define SIMPLE_HMM_H

#include "AudioToScoreAligner.h"

#include <vector>
#include <unordered_map>

using std::vector;
using std::pair;


class SimpleHMM
{
public:
    SimpleHMM(AudioToScoreAligner& aligner); // not const bc of getLikelihood()
    ~SimpleHMM();

    struct State {
        int eventIndex; // -1 means before first event; -2 means after last event
        int microIndex; // index of this microstate within the event
        unordered_map<State*, double> nextStates; // value is the transition prob
        unordered_map<State*, double> prevStates;

        State(int e, int m) : eventIndex{e}, microIndex{m} { }

        static string toString(State s) {
            string ss = to_string(s.eventIndex) + "\t" + to_string(s.microIndex);
            for (auto& p : s.nextStates) {
                ss += "\t next: " + to_string(p.first->eventIndex) +
                 " " + to_string(p.first->microIndex) + " " + to_string(p.second);
            }
            return ss;
        }

        // Ignore nextStates when comparing.
        bool operator==(const State &other) const {
            if (eventIndex == other.eventIndex && microIndex == other.microIndex)
                return true;
            return false;
        }

        bool operator<(const State& other) const {
            if (eventIndex < other.eventIndex)  return true;
            else if (eventIndex > other.eventIndex) return false;
            // equal eventIndex:
            if (microIndex < other.microIndex)  return true;
            else if (microIndex > other.microIndex) return false;
            // equal eventIndex and equal microIndex:
            return false;
        }

        bool operator>(const State& other) const {
            return !( (*this < other) || (*this == other) );
        }
    };

    typedef State StateGraph; // Starting State (eventIndex = -1)

    struct Hypothesis {
        const State& state;
        double prob; // might be in log
        Hypothesis(const State& s, double p) : state{s}, prob{p} { }

        bool operator==(const Hypothesis &other) const {
            if (state == other.state && prob == other.prob)
                return true;
            return false;
        }

        // Compare prob first; if equal, compare state.
        bool operator<(const Hypothesis& other) const {
            if (prob < other.prob)  return true;
            else if (prob > other.prob) return false;
            // equal prob:
            return state < other.state;
        }

        bool operator>(const Hypothesis& other) const {
            return !( (*this < other) || (*this == other) );
        }
    };

    AudioToScoreAligner::AlignmentResults getAlignmentResults();

private:
    AudioToScoreAligner m_aligner;
    StateGraph m_startingState = State(-1, 0); // YJ: Is there a better way?
};

#endif
