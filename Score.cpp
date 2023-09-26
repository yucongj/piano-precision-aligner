/*
  Yucong Jiang, June 2021
*/
#include "Score.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>






using namespace std;

Score::Score() : m_timeSignatureNumer{4}, m_timeSignatureDenom{4}
{
}

Score::~Score()
{
}

// The last event in scoreFilePath is ignored (all out notes).
bool Score::initialize(string scoreFilePath)
{
    string line;
    string currentMeasure = "";
    vector<Note> continuingNotes;
    MeasureInfo mi(0, Fraction(), Fraction());
    MusicalEvent currentEvent(mi); // to be replaced later

    ifstream scoreFile(scoreFilePath);
    if (!scoreFile.good()) {
        cerr<<"Cannot open file "<<scoreFilePath<<"\n";
        return false;
    }

    while (getline(scoreFile, line)) {
        istringstream iss(line);

        // measure+Position
        string measureAndPosition;
        getline(iss, measureAndPosition, '\t');
        string mn, mp;
        istringstream measureString(measureAndPosition);
        getline(measureString, mn, '+');
        getline(measureString, mp, '+');
        int measureNumber = stoi(mn);
        Fraction measurePosition = Fraction::fromString(mp);

        // measure
        string m;
        getline(iss, m, '\t');
        Fraction measureFraction = Fraction::fromString(m);

        // skip
        string placeholder;
        getline(iss, placeholder, '\t');

        // midi number
        string midiString;
        getline(iss, midiString, '\t');
        int midi = stoi(midiString);

        // in: non-zero; out: 0
        string s;
        getline(iss, s, '\t');
        int velocity = stoi(s);


        if (m != currentMeasure) { // new event

            if (currentMeasure != "") { // skip the beginning measure
                // Add continuing notes, if any, from the last event.
                for (Note note: continuingNotes)
                    currentEvent.notes.push_back(Note(false, note.midiNumber));
                currentEvent.duration = measureFraction - currentEvent.measureInfo.measureFraction;
                m_musicalEvents.push_back(currentEvent);
                continuingNotes = currentEvent.notes;
            }

            currentEvent = MusicalEvent(MeasureInfo(
                measureNumber, measurePosition, measureFraction));
            currentMeasure = m;
        }

        // If it's an out note, delete it from continuingNotes.
        if (velocity == 0) {
            for (vector<Note>::iterator it = continuingNotes.begin();
             it != continuingNotes.end(); it++) {
                if (it->midiNumber == midi) {
                    continuingNotes.erase(it);
                    break;
                }
            }
        } else // If it's an in note, add it to the currentEvent.
            currentEvent.notes.push_back(Note(true, midi));
    }

    return true;
}


// Read in the tempo information and assign a tempo value for each event in MusicalEvent
bool Score::readTempo(string tempoFilePath)
{
    string line;
    MeasureInfo mi(0, Fraction(), Fraction());
    TempoChange currentTempo(mi, 120.0, 1.0); // to be replaced later

    ifstream tempoFile(tempoFilePath);
    if (!tempoFile.good()) {
        cerr<<"Cannot open file "<<tempoFilePath<<"\n";
        return false;
    }

    while (getline(tempoFile, line)) {
        istringstream iss(line);

        // measure+Position
        string measureAndPosition;
        getline(iss, measureAndPosition, '\t');
        string mn, mp;
        istringstream measureString(measureAndPosition);
        getline(measureString, mn, '+');
        getline(measureString, mp, '+');
        int measureNumber = stoi(mn);
        Fraction measurePosition = Fraction::fromString(mp);

        // tempo value
        string tempoString;
        getline(iss, tempoString, '\t');
        float tempo = stof(tempoString);

        // note length for the tempo
        string nlString;
        getline(iss, nlString, '\t');
        float noteLength = stof(nlString);

        m_tempoChanges.push_back(TempoChange(MeasureInfo(measureNumber,
         measurePosition, measurePosition), tempo, noteLength)); // dummy measureFraction
    }
    // Set all events with the default tempo
    for (int count = 0; count < m_musicalEvents.size(); count++) {
        m_musicalEvents[count].tempo = 120.; // default tempo is "quarter note = 120."
    }

    // Apply any tempo changes to relevant events
    for (int count = 0; count + 1 < m_tempoChanges.size(); count++) {
        TempoChange start = m_tempoChanges[count];
        TempoChange end = m_tempoChanges[count+1];
        for (auto &event: m_musicalEvents)
            if (event.measureInfo >= start.measureInfo && event.measureInfo < end.measureInfo)
                event.tempo = start.newTempo * start.noteLength;
    }
    if (m_tempoChanges.size() > 0) {
        TempoChange last = m_tempoChanges[m_tempoChanges.size()-1];
        for (auto &event: m_musicalEvents)
            if (event.measureInfo >= last.measureInfo)
                event.tempo = last.newTempo * last.noteLength;
    }
    // testing:
    for (auto &event: m_musicalEvents) {
        cerr<<"***TEMPO: "<<event.measureInfo.measureNumber<<"+"
                 <<event.measureInfo.measurePosition<<" -> "<<event.tempo<<endl;
    }

    return true;
}

std::ostream& operator<<(std::ostream &strm, Fraction &f) {
   return strm << to_string(f.numerator) << "/" << to_string(f.denominator);
}

const Score::MusicalEventList& Score::getMusicalEvents() const
{
    return m_musicalEvents;
}

int Score::getTimeSignatureNumer() const
{
    return m_timeSignatureNumer;
}

int Score::getTimeSignatureDenom() const
{
    return m_timeSignatureDenom;
}

void Score::setEventTemplates(NoteTemplates& t)
{
    int bins = t[60].size();
    if (bins <= 0) {
        std::cerr << "setEventTemplates: Something is wrong with the note templates." << '\n';
        return;
    }
    double smallValue = 1 / (double)bins;
    double backgroundPortion = 0.05;

    for (auto &event: m_musicalEvents) {
        event.eventTemplate.resize(bins, 0);
        for (const auto &note: event.notes) {
            int midi = note.midiNumber;
            for (int k = 0; k < bins; k++) {
                event.eventTemplate[k] += t[midi][k];
            }
        }
        // Normalize:
        double total = 0;
        for (const auto &value: event.eventTemplate) {
            total += value;
        }
        if (total == 0) {
            for (auto &value: event.eventTemplate) {
                value = smallValue;
            }
        } else {
            for (auto &value: event.eventTemplate) {
                value /= total;
            }
            for (auto &value: event.eventTemplate) {
                value = smallValue*backgroundPortion + value*(1-backgroundPortion);
            }
        }
    }
}

/*
int main()
{
    Score score;
    string testScorePath = "/Users/yjiang3/Desktop/Pilot/testingScores/Barcarolle.solo";
    if (score.initialize(testScorePath)) {
        Score::MusicalEventList events = score.getMusicalEvents();
        // Testing code:
        for (Score::MusicalEvent event: events) {
            cout<<"***MEASURE: "<<event.measureInfo.measureFraction<<endl;
            std::cout << "duration: " << event.duration << '\n';
            for (Score::Note note: event.notes) {
                cout<<note.isNewNote<<"\t"<<note.midiNumber<<endl;
            }
        }
    } else {
        cerr<<"Error when reading the score file: "<<testScorePath<<"\n";
    }
}
*/
