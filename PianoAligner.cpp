/*
  modified by Yucong Jiang, June 2021
*/

#include "PianoAligner.h"
#include "AudioToScoreAligner.h"

#include "Templates.h"
#include "Paths.h"
#include "Score.h" // delete later?
#include <cmath> // delete later
#include <filesystem>

using namespace std;

PianoAligner::PianoAligner(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_aligner(nullptr),
    m_blockSize(0),
    m_scorePositionStart_numerator(-1.f),
    m_scorePositionStart_denominator(-1.f),
    m_scorePositionEnd_numerator(-1.f),
    m_scorePositionEnd_denominator(-1.f),
    m_audioStart_sec(-1.f),
    m_audioEnd_sec(-1.f),
    m_isFirstFrame(true),
    m_frameCount(0)
{
}

PianoAligner::~PianoAligner()
{
    delete m_aligner;
}

string
PianoAligner::getIdentifier() const
{
    return "pianoaligner";
}

string
PianoAligner::getName() const
{
    return "Piano Aligner";
}

string
PianoAligner::getDescription() const
{
    // Return something helpful here!
    return "An audio-to-score alignment plugin created by YJ.";
}

string
PianoAligner::getMaker() const
{
    // Your name here
    return "Yucong Jiang";
}

int
PianoAligner::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string
PianoAligner::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "Copyright (2021). All Rights Reserved";
}

PianoAligner::InputDomain
PianoAligner::getInputDomain() const
{
    return FrequencyDomain;
}

size_t
PianoAligner::getPreferredBlockSize() const
{
    //return 1024; // 0 means "I can handle any block size"
    return 1024*6;
}

size_t
PianoAligner::getPreferredStepSize() const
{
    //return 512; // 0 means "anything sensible"; in practice this
              // means the same as the block size for TimeDomain
              // plugins, or half of it for FrequencyDomain plugins
    return 128*6;//256*6;
}

size_t
PianoAligner::getMinChannelCount() const
{
    return 1;
}

size_t
PianoAligner::getMaxChannelCount() const
{
    return 1;
}

PianoAligner::ParameterList
PianoAligner::getParameterDescriptors() const
{
    ParameterList list;

    // If the plugin has no adjustable parameters, return an empty
    // list here (and there's no need to provide implementations of
    // getParameter and setParameter in that case either).

    // Note that it is your responsibility to make sure the parameters
    // start off having their default values (e.g. in the constructor
    // above).  The host needs to know the default value so it can do
    // things like provide a "reset to default" function, but it will
    // not explicitly set your parameters to their defaults for you if
    // they have not changed in the mean time.

    ParameterDescriptor d;

    d.identifier = "score-position-start-numerator";
    d.name = "Score Position - Start - Numerator";
    d.description = "";
    d.unit = "";
    d.minValue = -1.f;
    d.maxValue = 100000.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    d.identifier = "score-position-start-denominator";
    d.name = "Score Position - Start - Denominator";
    d.description = "";
    d.unit = "";
    d.minValue = -1.f;
    d.maxValue = 100000.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    d.identifier = "score-position-end-numerator";
    d.name = "Score Position - End - Numerator";
    d.description = "";
    d.unit = "";
    d.minValue = -1.f;
    d.maxValue = 100000.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    d.identifier = "score-position-end-denominator";
    d.name = "Score Position - End - Denominator";
    d.description = "";
    d.unit = "";
    d.minValue = -1.f;
    d.maxValue = 100000.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    d.identifier = "audio-start";
    d.name = "Audio - Start";
    d.description = "";
    d.unit = "s";
    d.minValue = -1.f;
    d.maxValue = 3600.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    d.identifier = "audio-end";
    d.name = "Audio - End";
    d.description = "";
    d.unit = "s";
    d.minValue = -1.f;
    d.maxValue = 3600.f;
    d.defaultValue = -1.f;
    d.isQuantized = false;
    list.push_back(d);

    return list;
}

float
PianoAligner::getParameter(string identifier) const
{
    if (identifier == "score-position-start-numerator") {
        return m_scorePositionStart_numerator;
    } else if (identifier == "score-position-start-denominator") {
        return m_scorePositionStart_denominator;
    } else if (identifier == "score-position-end-numerator") {
        return m_scorePositionEnd_numerator;
    } else if (identifier == "score-position-end-denominator") {
        return m_scorePositionEnd_denominator;
    } else if (identifier == "audio-start") {
        return m_audioStart_sec;
    } else if (identifier == "audio-end") {
        return m_audioEnd_sec;
    }
    return 0;
}

void
PianoAligner::setParameter(string identifier, float value)
{
    if (identifier == "score-position-start-numerator") {
        m_scorePositionStart_numerator = value;
    } else if (identifier == "score-position-start-denominator") {
        m_scorePositionStart_denominator = value;
    } else if (identifier == "score-position-end-numerator") {
        m_scorePositionEnd_numerator = value;
    } else if (identifier == "score-position-end-denominator") {
        m_scorePositionEnd_denominator = value;
    } else if (identifier == "audio-start") {
        m_audioStart_sec = value;
    } else if (identifier == "audio-end") {
        m_audioEnd_sec = value;
    }
}

PianoAligner::ProgramList
PianoAligner::getPrograms() const
{
    ProgramList list;

    auto scores = Paths::getScores();

    cerr << "PianoAligner::getPrograms: have " << scores.size() << " scores" << endl;

    for (auto score : scores) {
        cerr << "PianoAligner::getPrograms: score " << score.first << endl;
        list.push_back(score.first);
    }

    // If you have no programs, return an empty list (or simply don't
    // implement this function or getCurrentProgram/selectProgram)

    return list;
}

string
PianoAligner::getCurrentProgram() const
{
    return ""; // no programs
}

void
PianoAligner::selectProgram(string name)
{
    m_scoreName = name;
    cerr << "In selectProgram: name is -> " << name << '\n'; // ???
}

PianoAligner::OutputList
PianoAligner::getOutputDescriptors() const
{
    OutputList list;

    // See OutputDescriptor documentation for the possibilities here.
    // Every plugin must have at least one output.
    /*
    OutputDescriptor d;
    d.identifier = "chordonsets";
    d.name = "Chord Onsets";
    d.description = "Chord onsets by the dummy plugin";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);
    */
    OutputDescriptor d;
    d.identifier = "testsimplehmm";
    d.name = "Testing Simple HMM";
    d.description = "Testing for pianoaligner";
    d.unit = "";
    d.hasFixedBinCount = true;
    /*
    if (m_blockSize == 0) {
        //cerr << "m_blockSize is 0 in getOutputDescriptors()." << '\n';
        cerr << "Sample Rate is " << m_inputSampleRate<<'\n';
        d.binCount = (512*6)/2 + 1;
    } else {
        d.binCount = m_blockSize/2 + 1;
    }
    */
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    //d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = m_inputSampleRate/(128*6);
    list.push_back(d);

    // Testing:
    d.identifier = "testingtemplates";
    d.name = "Testing Templates";
    d.description = "Testing the templates";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1024/2;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    list.push_back(d);

    // Testing:
    d.identifier = "testingpowerspectrum";
    d.name = "Testing Power Spectrum";
    d.description = "Normalized values";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1024/2;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = m_inputSampleRate/(128*6);
    list.push_back(d);

    // Onsets:
    d.identifier = "audio-to-score-alignment";
    d.name = "Audio-to-score Alignment";
    d.description = "Audio-to-score alignment result by the plugin";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    // Tempo line:
    d.identifier = "eventtempo";
    d.name = "Event Tempo";
    d.description = "Tempo of an event";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);


    return list;
}

bool
PianoAligner::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    if (blockSize != 1024*6) {
	       return false;
    }
    if (stepSize != 128*6) { //256*6
	       return false;
    }

    m_isFirstFrame = true;
    
    // Real initialisation work goes here!

    m_aligner = new AudioToScoreAligner(m_inputSampleRate, stepSize, -1, -1, -1, -1); // -1 means aligment constraints not set yet
    m_blockSize = blockSize;

    if (m_scoreName == "") {
        // [cc] By default we don't run at all unless a score has been
        // chosen through selectProgram. This environment variable
        // provides a getout in the case where we need the default
        // configuration to be meaningful (e.g. when running in the
        // plugin tester).
        if (getenv("PIANO_ALIGNER_USE_DEFAULT_SCORE") != nullptr) {
            auto programs = getPrograms();
            if (programs.empty()) {
                cerr << "PianoAligner::initialise: No scores available" << endl;
                return false;
            } else {
                m_scoreName = programs[0];
            }
        } else {
            cerr << "PianoAligner::initialise: No score selected" << endl;
            return false;
        }
    }
    
    if (m_aligner->loadAScore(m_scoreName, blockSize)) {
	    return true;
    } else {
        cerr << "PianoAligner::initialise: Failed to load score "
		  << m_scoreName << endl;
	    return false;
    }
}

void
PianoAligner::reset()
{
    // Clear buffers, reset stored values, etc
    m_isFirstFrame = true;
}

PianoAligner::FeatureSet
PianoAligner::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    // Do actual work!

    if (m_isFirstFrame) {
        m_firstFrameTime = timestamp; // 0.064000000R in simple-host; 0.000000000R in SV
        m_isFirstFrame = false;
        m_frameCount = 0;
        cerr << "first frame time = "<<timestamp << '\n';
    }

    int scale = 6; // hard-coded for now
    int bins = (m_blockSize/scale)/2;
    const float *fbuf = inputBuffers[0];
    AudioToScoreAligner::DataSpectrum s;
    s.reserve(bins);
    double total = 0.;
    for (int i = 1; i <= bins; i++) { // skip DC
        double real = fbuf[i*2];
        double imag = fbuf[i*2 + 1];
        double power = real*real + imag*imag;
        s.push_back(power);
        total += power;
    }
    for (auto& value: s) {
        if (total != 0.) {
            value /= total;
        }
    }
    m_aligner->supplyFeature(s);
    m_frameCount++;

    // Testing templates:
    /*
    FeatureSet fs;
    Feature feature;
    feature.hasTimestamp = false;
    feature.values.reserve(bins); // optional
    long frame = Vamp::RealTime::realTime2Frame(timestamp, m_inputSampleRate);
    int index = floor(29.*frame/(m_inputSampleRate*12.)); // 0-based index
    Score::MusicalEventList events = m_aligner->getScore().getMusicalEvents();
    Template t = events[index].eventTemplate;
    for (int bin = 0; bin < bins; bin++) {
        feature.values.push_back(t[bin]);
    }
    fs[1].push_back(feature);

    return fs;
    */
    return FeatureSet();
}

PianoAligner::FeatureSet
PianoAligner::getRemainingFeatures()
{
    FeatureSet featureSet;
/*
    AudioToScoreAligner::AlignmentResults alignmentResults = m_aligner->align();
    for (const auto& result: alignmentResults) {
        Feature feature;
        feature.hasTimestamp = false;
        feature.values.push_back(result);
        // feature.timestamp = result;
        featureSet[0].push_back(feature);
    }
*/

    Score::MusicalEventList eventList = m_aligner->getScore().getMusicalEvents();
    // TODO: assert that eventList has at least one event
    int startEvent = 0;
    int endEvent = int(eventList.size()-1);
    int startFrame = 0;
    int endFrame = m_frameCount - 1;
    // Set alignment constraints through m_start/endEvent and m_start/endFrame
    int e = 0;

    if (fabs(m_scorePositionStart_numerator - -1.) > .0000001) { // find the starting event
        Fraction target = Fraction(m_scorePositionStart_numerator, m_scorePositionStart_denominator);
        float distance = fabs((target - eventList[e].measureInfo.measureFraction).getValue());
        while ( (e+1) < int(eventList.size()) && fabs((target - eventList[e+1].measureInfo.measureFraction).getValue()) < distance) {
            e++;
            distance = fabs((target - eventList[e].measureInfo.measureFraction).getValue());
        }
        startEvent = e;
    }
    if (fabs(m_scorePositionEnd_numerator - -1.) > .0000001) { // find the ending event
        Fraction target = Fraction(m_scorePositionEnd_numerator, m_scorePositionEnd_denominator);
        float distance = fabs((target - eventList[e].measureInfo.measureFraction).getValue());
        while ( (e+1) < int(eventList.size()) && fabs((target - eventList[e+1].measureInfo.measureFraction).getValue()) < distance) {
            e++;
            distance = fabs((target - eventList[e].measureInfo.measureFraction).getValue());
        }
        endEvent = e;
    }

    if (fabs(m_audioStart_sec - -1.) > .0000001) { // find the starting frame
        Vamp::RealTime t = Vamp::RealTime::fromSeconds(m_audioStart_sec);
        startFrame = Vamp::RealTime::realTime2Frame(t - m_firstFrameTime, m_inputSampleRate) / (128.*6.);
        cerr<<"***Calculated starting frame is: "<<startFrame<<"; t="<<t<<endl;
    }

    if (fabs(m_audioEnd_sec - -1.) > .0000001) { // find the ending frame
        Vamp::RealTime t = Vamp::RealTime::fromSeconds(m_audioEnd_sec);
        endFrame = Vamp::RealTime::realTime2Frame(t - m_firstFrameTime, m_inputSampleRate) / (128.*6.);
        cerr<<"***Calculated ending frame is: "<<endFrame<<"; t="<<t<<"; m_frameCount="<<m_frameCount<<endl;
    }

    Score::MeasureInfo info = eventList[startEvent].measureInfo;
    string label = info.toLabel();
    cerr<<"***Start label is: "<<label<<endl;
    info = eventList[endEvent].measureInfo;
    label = info.toLabel();
    cerr<<"***End label is: "<<label<<endl;
    cerr<<"***Start frame is: "<<startFrame<<"; start second = "<<m_audioStart_sec<<"; m_firstFrameTime = "<<m_firstFrameTime<<endl;
    cerr<<"***End frame is: "<<endFrame<<"; end second = "<<m_audioEnd_sec<<endl;
    m_aligner->setAlignmentConstraints(startEvent, endEvent, startFrame, endFrame);

    // Window version:
    vector<int> frames;
    AudioToScoreAligner::AlignmentResults alignmentResults = m_aligner->align();
    int event = startEvent;
    for (const auto& frame: alignmentResults) {
        Feature feature;
        feature.hasTimestamp = true;
        feature.timestamp = m_firstFrameTime + Vamp::RealTime::frame2RealTime(frame*(128.*6.), m_inputSampleRate);
        cerr <<"event="<<event<< ", real time = "<<feature.timestamp << '\n';
        Score::MeasureInfo info = eventList[event].measureInfo;
        // Calculate label:
        feature.label = info.toLabel();
        // feature.values.push_back(info.measureFraction.getValue());
        featureSet[3].push_back(feature);
        frames.push_back(frame); // this value currently is not used anywhere, just for debugging.
        event++;
    }

/*
    // Show onsets. TODO: deal with this part in SimpleHMM instead of here.
    vector<int> frames;
    int currentEvent = -1;
    int frame = 0;
    for (const auto& result: alignmentResults) {
        if (result != currentEvent) {
            Feature feature;
            feature.hasTimestamp = true;
            feature.timestamp = m_firstFrameTime + Vamp::RealTime::frame2RealTime(frame, m_inputSampleRate/(128.*6.));
            cout <<"real time = "<< feature.timestamp << '\n';
            feature.label = to_string(result);
            featureSet[3].push_back(feature);
            currentEvent = result;
            frames.push_back(frame);
        }
        frame++;
    }
    */

/* Commented out on Nov 20, 2023
    // Show local tempo. TODO: deal with this part in SimpleHMM instead of here.
    for (int i = 0; i + 1 < int(frames.size()); i++) {
        Feature feature;
        feature.hasTimestamp = true;
        feature.timestamp = Vamp::RealTime::frame2RealTime(frames[i]*(128.*6.), m_inputSampleRate);//featureSet[3][i];
        double tempo = 100./(double)(frames[i+1] - frames[i]); // TODO: check != 0
        feature.values.push_back(tempo);
        featureSet[4].push_back(feature);
    }


    // Testing: plot "normalized" PowerSpectrum
    int scale = 6; // hard-coded for now
    int bins = (m_blockSize/scale)/2;
    double max = 0.;
    double min = 100000000.; // a large value
    double p = 0;
    for (const auto& spectrum: m_aligner->getDataFeatures()) { // get max and min
        for (int b = 0; b < bins; b++) {
            p = spectrum[b];
            if (p > max)    max = p;
            if (p < min)    min = p;
        }
    }
    double range = max-min; // Shouldn't be zero, but need to check
    cout << "max = "<<max<<", min="<<min << '\n';
    for (const auto& spectrum: m_aligner->getDataFeatures()) {
        Feature feature;
        feature.hasTimestamp = false;
        feature.values.reserve(bins); // optional
        for (int b = 0; b < bins; b++) {
            p = spectrum[b];
            feature.values.push_back(pow(log(1+p), 0.4));
            //feature.values.push_back((p-min)/range);
        }
        featureSet[2].push_back(feature);
    }
*/


    //Testing note templates:
/*
    cout<<"m_blockSize = "<<(m_blockSize / 2)<<"\n";
    NoteTemplates t = CreateNoteTemplates::getNoteTemplates(
        m_inputSampleRate, m_blockSize);
    for (auto &pair : t) {
        int midi = pair.first;
        Template &spect = pair.second;
        cout<<"### MIDI: "<<midi<<" ###"<<"\n";
        for (auto value: spect) {
            cout<<value<<",";
        }
        cout<<"\n";
    }
*/
/*
    //Testing event templates:
    int index = 1;
    for (const auto &event: m_aligner->getScore().getMusicalEvents()) {
        cout<<"### Event: "<<index<<" ###"<<"\n";
        for (const auto &value: event.eventTemplate) {
            cout<<value<<",";
        }
        cout<<"\n";
        index++;
    }
*/
    return featureSet;
}
