#include "Synth.h"

#include <JuceHeader.h>

SynthVoice::SynthVoice() {
  auto sr = getSampleRate();
  carrOsc = new SinOsc();
  carrOsc->setSampleRate(sr);
  carrOsc->setDefFreq(carrFreq);

  midiOsc = new SinOsc();
  midiOsc->setSampleRate(sr);

  env.setSampleRate(sr);
  setADSR(0.1f, 0.1f, 1.0f, 0.2f);
}
SynthVoice::~SynthVoice() {
  delete carrOsc;
  delete midiOsc;
}
void SynthVoice::startNote(int midiNoteNumber, float velocity,
                           juce::SynthesiserSound* sound,
                           int currentPitchWheelPosition) {
  auto freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  midiOsc->setFreq(freq);
  env.noteOn();
  playing = true;
  isOff = false;
}
void SynthVoice::stopNote(float velocity, bool allowTailOff) {
  env.noteOff();
  isOff = true;
}
void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                 int startSample, int numSamples) {
  if (playing) {
    for (auto i = startSample; i < (startSample + numSamples); i++) {
      float currentSample;
      // Modulation Part
      if (moduType == 0) {  // No Modulation
        currentSample = midiOsc->getNextSample();
      } else if (moduType == 1) {  // FM
        carrOsc->setFreq(carrOsc->getDefFreq() *
                         (1 + midiOsc->getNextSample()));
        currentSample = carrOsc->getNextSample();
      } else if (moduType == 2) {  // PM
        currentSample = carrOsc->getShiftedSample(midiOsc->getNextSample());
      } else if (moduType == 3) {  // AM
        currentSample =
            carrOsc->getNextSample() * (1 + midiOsc->getNextSample());
      }
      // Envelope
      currentSample *= env.getNextSample();
      // Gain
      currentSample *= 0.2;
      // Render
      for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++) {
        outputBuffer.addSample(ch, i, currentSample);
      }
      // Check if this note should be cleaned
      if (isOff && !env.isActive()) {
        playing = false;
        clearCurrentNote();
        break;
      }
    }
  }
}
bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<SynthSound*>(sound) != nullptr;
}
void SynthVoice::setCarrFreq(float cf) {
  carrFreq = cf;
  carrOsc->setDefFreq(carrFreq);
}
void SynthVoice::setADSR(float a, float d, float s, float r) {
  envPara.attack = a;
  envPara.decay = d;
  envPara.sustain = s;
  envPara.release = r;
  env.setParameters(envPara);
}
void SynthVoice::setModuType(int mt) {
  moduType = mt;
  carrOsc->clear();  // Reset the carrier osc
}
