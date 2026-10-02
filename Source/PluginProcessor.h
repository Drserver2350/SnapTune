#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "TuneDsp.h"

class SnapTuneProcessor : public juce::AudioProcessor {
public:
    SnapTuneProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SnapTune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    tune::PitchDetector detector;
    tune::PitchShifter shifters[2];
    double sr = 44100.0;
    float currentRatio = 1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnapTuneProcessor)
};
