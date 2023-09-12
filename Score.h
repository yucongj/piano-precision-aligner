/*
  Yucong Jiang, June 2021
*/

#ifndef SCORE_H
#define SCORE_H

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "Templates.h"


using std::istringstream;
using std::vector;
using std::stoi;
using std::string;
using std::to_string;


struct Fraction // Simplest form using GCD.
{
    int numerator;
    int denominator;

    Fraction() : numerator{0}, denominator{1} { }
    Fraction(int n, int d) {
        int div = gcd(n, d);
        numerator = n/div;
        denominator = d/div;
    }

    static int gcd(int p, int q) {
        if (q == 0) {
            return p;
        } else {
            return gcd(q, p % q);
        }
    }

    static Fraction fromString(const string &s) {
        string n, d;
        istringstream iss(s);
        getline(iss, n, '/');
        getline(iss, d, '/');
        return Fraction(stoi(n), stoi(d));
    }

    double getValue() const {
        if (denominator == 0) {
            std::cerr<<"denominator is zero in Fraction::getValue()."<<"\n";
            return 0;
        }
        return (double)numerator / (double)denominator;
    }

    bool operator==(const Fraction &other) const { // default c++20
        if (numerator == other.numerator && denominator == other.denominator)
            return true;
        return false;
    }

    Fraction(const Fraction&) = default;

    Fraction& operator=(const Fraction&) = default;

    bool operator<(const Fraction& other) const {
        int n = numerator;
    	int d = denominator;
    	int nn = other.numerator;
    	int dd = other.denominator;
    	return (n * dd < nn * d);
    }

    Fraction operator-(const Fraction& other) const {
        int n = numerator;
    	int d = denominator;
    	int nn = other.numerator;
    	int dd = other.denominator;
    	return (Fraction(n*dd - d*nn, d*dd));
    }
};

std::ostream& operator<<(std::ostream &strm, Fraction &f);

class Score
{
public:
    Score();
    ~Score();

    struct MeasureInfo
    {
        int measureNumber;
        Fraction measurePosition;
        Fraction measureFraction; // sometimes with a dummy value

        MeasureInfo(int mn, Fraction mp, Fraction mf): measureNumber{mn},
         measurePosition{mp}, measureFraction{mf} { }
        
        bool operator==(const MeasureInfo &other) const { // default c++20
            if (measureNumber == other.measureNumber &&
             measurePosition == other.measurePosition)
                return true;
            return false;
        }

        bool operator<(const MeasureInfo& other) const { // ignore measureFraction
            if (measureNumber < other.measureNumber) return true;
            else if(measureNumber > other.measureNumber) return false;
            else return measurePosition < other.measurePosition;
        }

        bool operator>=(const MeasureInfo& other) const { // ignore measureFraction
            return (*this == other) || (other < *this);
        }
    };

    struct Note
    {
        bool isNewNote; // is an emerging note as opposed to a continuing note
        int midiNumber;

        Note(bool nn, int mn) : isNewNote{nn}, midiNumber{mn} { }
    };

    struct MusicalEvent
    {
        MeasureInfo measureInfo;
        vector<Note> notes;
        Template eventTemplate;
        Fraction duration;
        float tempo; // e.g., Quarter note = 120.0

        MusicalEvent(MeasureInfo mi) : measureInfo{mi} { }
    };

    struct TempoChange
    {
        // Example on scores: quarter note = 120 bpm
        MeasureInfo measureInfo;
        float newTempo;
        float noteLength; // Quarter note = 1.0, eighth note = 0.5, etc.

        TempoChange(MeasureInfo mi, float t, float n) : measureInfo{mi},
         newTempo{t}, noteLength{n} { }
    };

    struct Meter
    {
        int measureNumber;
        Fraction meter;

        Meter(int mn, Fraction meter): measureNumber{mn},
         meter{meter} { }
    };

    typedef vector<MusicalEvent> MusicalEventList;
    typedef vector<TempoChange> TempoChangeList;
    typedef vector<Meter> MeterList;

    bool initialize(string scoreFilePath);
    bool readTempo(string tempoFilePath);

    const MusicalEventList& getMusicalEvents() const;
    double getDefaultTempo() const; // delete
    int getTimeSignatureNumer() const; // delete
    int getTimeSignatureDenom() const; // delete

    void setEventTemplates(NoteTemplates& t);

private:
    int m_timeSignatureNumer; // TODO: read the .meter file
    int m_timeSignatureDenom; // TODO: read the .meter file
    double m_defaultTempo; // TODO: incorporate tempo changes in alignment algorithm
    MusicalEventList m_musicalEvents;
    TempoChangeList m_tempoChanges;
    MeterList m_meters;
};

#endif