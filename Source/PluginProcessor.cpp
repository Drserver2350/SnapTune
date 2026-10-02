#include "PluginProcessor.h"

SnapTuneProcessor::SnapTuneProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout SnapTuneProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<AudioParameterChoice>(ParameterID{"key", 1}, "Key",
        StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    l.add(std::make_unique<AudioParameterChoice>(ParameterID{"scale", 1}, "Scale",
        StringArray{"Chromatic","Major","Minor","Major Pentatonic","Minor Pentatonic","Blues"}, 0));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"speed", 1}, "Retune Speed (ms)",
        NormalisableRange<float>(0.0f, 400.0f, 1.0f, 0.4f), 20.0f));   // 0 = hard robot snap
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"amount", 1}, "Correction Amount",
        NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"mix", 1}, "Mix",
        NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"sens", 1}, "Detection Sensitivity",
        NormalisableRange<float>(0.05f, 0.4f), 0.15f));
    return l;
}

bool SnapTuneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == layouts.getMainInputChannelSet();
}

void SnapTuneProcessor::prepareToPlay(double sampleRate, int) {
    sr = sampleRate;
    // window large enough to see ~70 Hz at any sample rate
    int win = 1024; while (win < sampleRate / 70.0 * 2.2) win *= 2;
    detector.prepare(sampleRate, win);
    for (auto& s : shifters) s.prepare(sampleRate);
    currentRatio = 1.0f;
}

void SnapTuneProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    const int   root   = (int) apvts.getRawParameterValue("key")->load();
    const auto  scale  = (tune::Scale) (int) apvts.getRawParameterValue("scale")->load();
    const float speed  = apvts.getRawParameterValue("speed")->load();
    const float amount = apvts.getRawParameterValue("amount")->load();
    const float mix    = apvts.getRawParameterValue("mix")->load();
    detector.threshold = apvts.getRawParameterValue("sens")->load();

    const float alpha = speed <= 0.0f ? 1.0f : 1.0f - std::exp(-1.0f / (speed * 0.001f * (float) sr));
    const int hop = juce::jmax(64, (int) (sr / 172.0));

    for (int i = 0; i < n; ++i) {
        float mono = 0.0f;
        for (int c = 0; c < numCh; ++c) mono += buffer.getSample(c, i);
        mono /= (float) numCh;

        float hz = detector.push(mono, hop);
        float target = 1.0f;
        if (hz > 0.0f) {
            float snapped = tune::snapToScale(hz, root, scale);
            target = std::pow(snapped / hz, amount);
            target = juce::jlimit(0.5f, 2.0f, target);
        }
        currentRatio += alpha * (target - currentRatio);

        for (int c = 0; c < numCh; ++c) {
            float dry = buffer.getSample(c, i);
            float wet = shifters[c].process(dry, currentRatio);
            buffer.setSample(c, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void SnapTuneProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void SnapTuneProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SnapTuneProcessor(); }
