/*
  modified by Yucong Jiang, June 2021
*/

#include "PianoAligner.h"
#include "AudioToScoreAligner.h"

#include "Templates.h"
#include "Paths.h"
#include "Score.h" // delete later
#include <cmath> // delete later
#include <filesystem>


PianoAligner::PianoAligner(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_blockSize(0),
    m_aligner(nullptr),
    m_isFirstFrame(true),
    m_frameCount(0)
    // Also be sure to set your plugin parameters (presumably stored
    // in member variables) to their default values here -- the host
    // will not do that for you
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
    return "A dummy plugin created by YJ.";
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
    d.identifier = "parameter";
    d.name = "Some Parameter"; // "Choice of score"
    d.description = "";
    d.unit = "";
    d.minValue = 0;
    d.maxValue = 10; // 4
    d.defaultValue = 5; // 0
    d.isQuantized = false; // true
    /*
    d.quantizeStep = 1; // added
    d.valueNames.push_back("C major scale");
    d.valueNames.push_back("BEAM_SEARCH_WIDTH");
    d.valueNames.push_back("C major scale right hand");
    d.valueNames.push_back("D major scale");
    d.valueNames.push_back("E major scale");
    */
    list.push_back(d);

    return list;
}

float
PianoAligner::getParameter(string identifier) const
{
    if (identifier == "parameter") { // if (identifier == "score")
        return 5; // return the ACTUAL current value of your parameter here!
    }
    return 0;
}

void
PianoAligner::setParameter(string identifier, float value)
{
    if (identifier == "parameter") {
        // set the actual value of your parameter
    }
}

PianoAligner::ProgramList
PianoAligner::getPrograms() const
{
    ProgramList list;

    auto scores = Paths::getScores();

    std::cerr << "PianoAligner::getPrograms: have " << scores.size() << " scores" << std::endl;

    for (auto score : scores) {
        std::cerr << "PianoAligner::getPrograms: score " << score.first << std::endl;
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
    std::cerr << "In selectProgram: name is -> " << name << '\n'; // ???
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
        //std::cerr << "m_blockSize is 0 in getOutputDescriptors()." << '\n';
        std::cerr << "Sample Rate is " << m_inputSampleRate<<'\n';
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
    d.identifier = "chordonsets";
    d.name = "Chord Onsets";
    d.description = "Chord onsets by the dummy plugin";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
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

    m_aligner = new AudioToScoreAligner(m_inputSampleRate, stepSize);
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
                std::cerr << "PianoAligner::initialise: No scores available" << std::endl;
                return false;
            } else {
                m_scoreName = programs[0];
            }
        } else {
            std::cerr << "PianoAligner::initialise: No score selected" << std::endl;
            return false;
        }
    }
    
    if (m_aligner->loadAScore(m_scoreName, blockSize)) {
	    return true;
    } else {
        std::cerr << "PianoAligner::initialise: Failed to load score "
		  << m_scoreName << std::endl;
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
        std::cerr << "first frame time = "<<timestamp << '\n';
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


/*
    if (m_frameCount >= 390 && m_frameCount <=403) {
        std::cerr << "#Frame == " <<m_frameCount;
        std::cerr << timestamp << '\n';
        for (const auto& value: s) {
            std::cerr << value << ',';
        }
        std::cerr << "end of Frame "<<m_frameCount << '\n'<<'\n';
    }

    m_frameCount++;
    // std::cout << "m_frameCount: " <<m_frameCount<< '\n';
*/

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


    // Window version:
    vector<int> frames;
    AudioToScoreAligner::AlignmentResults alignmentResults = m_aligner->align();
    Score::MusicalEventList eventList = m_aligner->getScore().getMusicalEvents();
    int event = 0;
    int lastChange = 0; // last event index that defines a new tempo
    float lastChangeTick = 0; // tick for the last event that defines a new tempo
    float lastTempo = 0; // will be set in the for loop below
    float currentTick = 0;

    for (const auto& frame: alignmentResults) {
        Feature feature;
        feature.hasTimestamp = true;
        feature.timestamp = m_firstFrameTime + Vamp::RealTime::frame2RealTime(frame*(128.*6.), m_inputSampleRate);
        std::cerr <<"event="<<event<< ", real time = "<<feature.timestamp << '\n';
        Score::MeasureInfo info = eventList[event].measureInfo;
        // Calculate label:
        feature.label = to_string(info.measureNumber);
        feature.label += "+" + to_string(info.measurePosition.numerator) + "/" + to_string(info.measurePosition.denominator);
        
        // Calculate tick:
        // TODO: check divide-by-zero for eventList[event].temp and info.measureFraction.denominator
        if (event == 0) {
            lastTempo = eventList[0].tempo;
            lastChangeTick = 0;
        } else {
            float currentTempo = eventList[event].tempo;
            Fraction duration = info.measureFraction - eventList[lastChange].measureInfo.measureFraction;
            currentTick = lastChangeTick + duration.numerator * 2000. * (120. / lastTempo) / duration.denominator;
            if (abs(currentTempo - lastTempo) > 0.001) { // encountering a tempo change
                lastTempo = currentTempo;
                lastChange = event;
                lastChangeTick = currentTick;
            }
        }
        std::cerr<<"***TICKS: "<<feature.label<<" -> "<<currentTick<<std::endl;
        // feature.values.push_back(info.measureFraction.numerator * 2000 / info.measureFraction.denominator);
        feature.values.push_back(currentTick);
        featureSet[3].push_back(feature);
        frames.push_back(frame);
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
            std::cout <<"real time = "<< feature.timestamp << '\n';
            feature.label = to_string(result);
            featureSet[3].push_back(feature);
            currentEvent = result;
            frames.push_back(frame);
        }
        frame++;
    }
    */

    // Show local tempo. TODO: deal with this part in SimpleHMM instead of here.
    for (int i = 0; i + 1 < frames.size(); i++) {
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
    std::cout << "max = "<<max<<", min="<<min << '\n';
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



    //Testing note templates:
/*
    std::cout<<"m_blockSize = "<<(m_blockSize / 2)<<"\n";
    NoteTemplates t = CreateNoteTemplates::getNoteTemplates(
        m_inputSampleRate, m_blockSize);
    for (auto &pair : t) {
        int midi = pair.first;
        Template &spect = pair.second;
        std::cout<<"### MIDI: "<<midi<<" ###"<<"\n";
        for (auto value: spect) {
            std::cout<<value<<",";
        }
        std::cout<<"\n";
    }
*/
/*
    //Testing event templates:
    int index = 1;
    for (const auto &event: m_aligner->getScore().getMusicalEvents()) {
        std::cout<<"### Event: "<<index<<" ###"<<"\n";
        for (const auto &value: event.eventTemplate) {
            std::cout<<value<<",";
        }
        std::cout<<"\n";
        index++;
    }
*/
    return featureSet;
}
