/*
  modified by Yucong Jiang, June 2021
*/

#include "PianoAligner.h"
#include "AudioToScoreAligner.h"

#include "Templates.h"
#include "Score.h" // delete later
#include <cmath> // delete later


PianoAligner::PianoAligner(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_blockSize(0),
    m_aligner(nullptr)
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
    return 256*6;
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
    d.name = "Some Parameter";
    d.description = "";
    d.unit = "";
    d.minValue = 0;
    d.maxValue = 10;
    d.defaultValue = 5;
    d.isQuantized = false;
    list.push_back(d);

    return list;
}

float
PianoAligner::getParameter(string identifier) const
{
    if (identifier == "parameter") {
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
    d.name = "Testing: Simple HMM";
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
    d.sampleRate = m_inputSampleRate/(256*6);
    list.push_back(d);

    // Testing:
    d.identifier = "testingtemplates";
    d.name = "Testing Templates";
    d.description = "Teing the templates";
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
    d.sampleRate = m_inputSampleRate/(256*6);
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
    if (stepSize != 256*6) {
	       return false;
    }


    // Real initialisation work goes here!

    m_aligner = new AudioToScoreAligner(m_inputSampleRate, stepSize);
    m_blockSize = blockSize;
    m_aligner->loadAScore(blockSize);

    return true;
}

void
PianoAligner::reset()
{
    // Clear buffers, reset stored values, etc
}

PianoAligner::FeatureSet
PianoAligner::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    // Do actual work!

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
        //Check total != 0
        value /= total;
    }
    m_aligner->supplyFeature(s);

    // Testing templates:
    FeatureSet fs;
    Feature feature;
    feature.hasTimestamp = false;
    feature.values.reserve(bins); // optional
    long frame = Vamp::RealTime::realTime2Frame(timestamp, m_inputSampleRate);
    int index = floor(29.*frame/(m_inputSampleRate*12.)); // 0-based index
    //std::cout << frame<<'\t'<<(m_inputSampleRate*12.)/(m_blockSize/2.) << '\n';
    Score::MusicalEventList events = m_aligner->getScore().getMusicalEvents();
    Template t = events[index].eventTemplate;
    //index = 21+floor(80.*frame/(m_inputSampleRate*12.));
    //std::cout <<"midi = "<<index << '\n';
    //NoteTemplates nt =
        //CreateNoteTemplates::getNoteTemplates(m_inputSampleRate, bins*2);
    //Template t = nt[index];
    for (int bin = 0; bin < bins; bin++) {
        feature.values.push_back(t[bin]);
    }
    fs[1].push_back(feature);


    return fs;
    //return FeatureSet();
}

PianoAligner::FeatureSet
PianoAligner::getRemainingFeatures()
{

    FeatureSet featureSet;
    AudioToScoreAligner::AlignmentResults alignmentResults = m_aligner->align();
    for (const auto& result: alignmentResults) {
        Feature feature;
        feature.hasTimestamp = false;
        feature.values.push_back(result);
        // feature.timestamp = result;
        featureSet[0].push_back(feature);
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
