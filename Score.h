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
        Fraction measureFraction;

        MeasureInfo(int mn, Fraction mp, Fraction mf): measureNumber{mn},
         measurePosition{mp}, measureFraction{mf} { }
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

        MusicalEvent(MeasureInfo mi) : measureInfo{mi} { }
    };

    typedef vector<MusicalEvent> MusicalEventList;

    bool initialize(string scoreFilePath);

    const MusicalEventList& getMusicalEvents() const;
    double getDefaultTempo() const;
    int getTimeSignatureNumer() const;
    int getTimeSignatureDenom() const;

    void setEventTemplates(NoteTemplates& t);

private:
    int m_timeSignatureNumer;
    int m_timeSignatureDenom;
    double m_defaultTempo;
    MusicalEventList m_musicalEvents;
};

#endif
