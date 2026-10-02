// Pure C++ DSP core (no JUCE dependency): YIN pitch detection + scale snapping + dual-tap pitch shifter.
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace tune {

// ---------- YIN pitch detector ----------
class PitchDetector {
public:
    void prepare(double sampleRate, int windowSize = 1024) {
        sr = sampleRate; N = windowSize; half = N / 2;
        ring.assign(N, 0.0f); frame.assign(N, 0.0f); yin.assign(half, 0.0f);
        pos = 0; counter = 0; lastHz = 0.0f;
    }
    // push one sample; re-analyses every `hop` samples. Returns latest pitch in Hz (0 = unvoiced).
    float push(float x, int hop = 256) {
        ring[pos] = x; pos = (pos + 1) % N;
        if (++counter >= hop) { counter = 0; analyse(); }
        return lastHz;
    }
    float threshold = 0.15f;
    float minHz = 70.0f, maxHz = 1000.0f;
private:
    void analyse() {
        float energy = 0.0f;
        for (int i = 0; i < N; ++i) { frame[i] = ring[(pos + i) % N]; energy += frame[i] * frame[i]; }
        if (energy / N < 1e-6f) { lastHz = 0.0f; return; }           // silence gate

        const int tauMin = std::max(2, (int) (sr / maxHz));
        const int tauMax = std::min(half - 1, (int) (sr / minHz));
        yin[0] = 1.0f; float running = 0.0f;
        for (int tau = 1; tau <= tauMax; ++tau) {
            float d = 0.0f;
            for (int j = 0; j < half; ++j) { float diff = frame[j] - frame[j + tau]; d += diff * diff; }
            running += d;
            yin[tau] = running > 0.0f ? d * tau / running : 1.0f;
        }
        int best = -1;
        for (int tau = tauMin; tau <= tauMax; ++tau) {
            if (yin[tau] < threshold) {
                while (tau + 1 <= tauMax && yin[tau + 1] < yin[tau]) ++tau;
                best = tau; break;
            }
        }
        if (best < 0) { lastHz = 0.0f; return; }
        float t = (float) best;                                        // parabolic refinement
        if (best > 1 && best < tauMax) {
            float a = yin[best - 1], b = yin[best], c = yin[best + 1];
            float den = a + c - 2.0f * b;
            if (std::abs(den) > 1e-9f) t += 0.5f * (a - c) / den;
        }
        lastHz = (float) (sr / t);
    }
    double sr = 44100.0; int N = 1024, half = 512, pos = 0, counter = 0;
    std::vector<float> ring, frame, yin; float lastHz = 0.0f;
};

// ---------- Scale snapping ----------
enum class Scale { Chromatic = 0, Major, Minor, MajorPentatonic, MinorPentatonic, Blues };

inline bool inScale(int pitchClass, int root, Scale s) {
    static const int masks[6][12] = {
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,1,0,1,1,0,1,0,1,0,1},
        {1,0,1,1,0,1,0,1,1,0,1,0},
        {1,0,1,0,1,0,0,1,0,1,0,0},
        {1,0,0,1,0,1,0,1,0,0,1,0},
        {1,0,0,1,0,1,1,1,0,0,1,0}};
    int rel = ((pitchClass - root) % 12 + 12) % 12;
    return masks[(int) s][rel] != 0;
}

// returns target frequency for detected hz
inline float snapToScale(float hz, int root, Scale s) {
    float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
    int centre = (int) std::lround(midi);
    float bestDist = 1e9f; int bestNote = centre;
    for (int k = -6; k <= 6; ++k) {
        int n = centre + k;
        if (!inScale(n % 12, root, s)) continue;
        float d = std::abs((float) n - midi);
        if (d < bestDist) { bestDist = d; bestNote = n; }
    }
    return 440.0f * std::pow(2.0f, (bestNote - 69) / 12.0f);
}

// ---------- Dual-tap crossfading delay-line pitch shifter ----------
class PitchShifter {
public:
    void prepare(double sampleRate) {
        W = std::max(256, (int) (sampleRate * 0.025));   // ~25 ms grain window
        buf.assign(W * 2 + 8, 0.0f); size = (int) buf.size(); wp = 0; phase = 0.0f;
    }
    float process(float in, float ratio) {
        buf[wp] = in;
        phase += (1.0f - ratio) / (float) W;
        phase -= std::floor(phase);
        float p2 = phase + 0.5f; p2 -= std::floor(p2);
        float out = tap(phase) * window(phase) + tap(p2) * window(p2);
        wp = (wp + 1) % size;
        return out;
    }
private:
    static float window(float p) { return 0.5f - 0.5f * std::cos(6.28318530718f * p); }
    float tap(float p) const {
        float delay = 1.0f + p * (float) W;
        float rp = (float) wp - delay;
        while (rp < 0.0f) rp += (float) size;
        int i0 = (int) rp; float frac = rp - (float) i0;
        int i1 = (i0 + 1) % size;
        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }
    std::vector<float> buf; int W = 1024, size = 0, wp = 0; float phase = 0.0f;
};

} // namespace tune
