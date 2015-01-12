// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * DSP functions and operations.
 */

#ifndef DSP_H_
#define DSP_H_

#include <memory>
#include <stdint.h>
#include <utility>
#include <vector>

using namespace std;

namespace radioreceiver {

/**
 * Type for sample block storage.
 */
typedef vector<float> Samples;

/**
 * A deinterlaced I/Q sample stream.
 */
struct SamplesIQ {
  Samples I;
  Samples Q;
};

/**
 * A small structure to contain stereo audio.
 */
struct StereoAudio {
  Samples left;
  Samples right;
  bool inStereo;
  bool carrier;
};

/**
 * Converts the given buffer of unsigned 8-bit samples into a samples object.
 * @param buffer A buffer containing the unsigned 8-bit samples.
 * @param length The buffer's length.
 * @param rate The buffer's sample rate.
 * @return The converted samples.
 */
Samples samplesFromUint8(uint8_t* buffer, int length);

/**
 * Converts the given buffer of signed 16-bit samples into a samples object.
 * @param buffer A buffer containing the signed 16-bit samples.
 * @param length The buffer's length.
 * @param rate The buffer's sample rate.
 * @return The converted samples.
 */
Samples samplesFromInt16(int16_t* buffer, int length);

/**
 * Generates coefficients for a FIR low-pass filter with the given
 * half-amplitude frequency and kernel length at the given sample rate.
 * @param sampleRate The signal's sample rate.
 * @param halfAmplFreq The half-amplitude frequency in Hz.
 * @param length The length of the coefficient array. Should be an odd number.
 * @return The filter coefficients.
 */
vector<float> getLowPassFIRCoeffs(int sampleRate, float halfAmplFreq,
                                  int length);

/**
 * A Finite Impulse Response filter.
 */
class FIRFilter {
  vector<float> coefficients_;
  Samples curSamples_;
  int step_;
  int offset_;

 public:
  /**
   * Constructor for an filter with the given coefficients and step interval.
   * @param coefficients The coefficients of the filter to apply.
   * @param step The stepping between samples.
   */
  FIRFilter(const vector<float>& coefficients, int step = 1);

  /**
   * Loads a new block of samples to filter.
   * @param samples The samples to load.
   */
  void loadSamples(const Samples& samples);

  /**
   * Returns a filtered sample.
   * @param index The index of the sample to return, corresponding
   *     to the same index in the latest sample block loaded via loadSamples().
   */
  float get(int index);
};

/**
 * A class to apply a low-pass filter and resample to a lower sample rate.
 */
class Downsampler {
  FIRFilter filter_;
  float rateMul_;

 public:
  /**
   * Constructor with the given input and output rate and filter coefficients.
   * @param inRate The input signal's sample rate.
   * @param outRate The output signal's sample rate.
   * @param coefficients The coefficients for the FIR filter to apply to the
   *     original signal before downsampling it.
   */
  Downsampler(int inRate, int outRate, const vector<float>& coefficients);

  /**
   * Returns a downsampled version of the given samples.
   * @param samples The sample block to downsample.
   * @return The downsampled block.
   */
  Samples downsample(const Samples& samples);
};

/**
 * A class to downsample and deinterlace an I/Q stream coming from the tuner.
 */
class IQDownsampler {
  FIRFilter filter_;
  float rateMul_;

 public:
  /**
   * Constructor with the given input and output rate and filter coefficients.
   * @param inRate The input signal's sample rate.
   * @param outRate The output signal's sample rate.
   * @param coefficients The coefficients for the FIR filter to apply to the
   *     original signal before downsampling it.
   */
  IQDownsampler(int inRate, int outRate, const vector<float>& coefficients);

  /**
   * Returns a downsampled version of the given samples.
   * @param samples The sample block to downsample.
   * @return The deinterlaced and downsampled block.
   */
  SamplesIQ downsample(const Samples& samples);
};

/**
 * A class to demodulate IQ-interleaved samples representing an amplitude
 * modulated signal into a raw audio signal.
 */
class AMDemodulator {
  IQDownsampler downsampler_;
  bool hasCarrier_;
 public:
  /**
   * Constructor for the given rates and maximum frequency deviation.
   * @param inRate The sample rate for the input signal.
   * @param outRate The sample rate for the output audio.
   * @param filterFreq The frequeny of the low-pass filter.
   * @param kernelLen The length of the filter kernel.
   */
  AMDemodulator(int inRate, int outRate, float filterFreq, int kernelLen);

  /**
   * Demodulates the given I/Q samples.
   * @param samples The samples to demodulate.
   * @return The demodulated sound.
   */
  Samples demodulateTuned(const Samples& samples);

  /**
   * Tells whether a carrier was detected in the last demodulated block.
   * @return Whether a carrier was detected.
   */
  bool hasCarrier();
};


/**
 * A class to demodulate IQ-interleaved samples representing a frequency
 * modulated signal into a raw audio signal.
 */
class FMDemodulator {
  float amplConv_;
  IQDownsampler downsampler_;
  float lI_;
  float lQ_;
  bool hasCarrier_;
 public:
  /**
   * Constructor for the given rates and maximum frequency deviation.
   * @param inRate The sample rate for the input signal.
   * @param outRate The sample rate for the output audio.
   * @param maxF The maximum frequency deviation.
   * @param filterFreq The frequeny of the low-pass filter.
   * @param kernelLen The length of the filter kernel.
   */
  FMDemodulator(int inRate, int outRate, int maxF, float filterFreq,
                int kernelLen);

  /**
   * Demodulates the given I/Q samples.
   * @param samples The samples to demodulate.
   * @return The demodulated sound.
   */
  Samples demodulateTuned(const Samples& samples);

  /**
   * Tells whether a carrier was detected in the last demodulated block.
   * @return Whether a carrier was detected.
   */
  bool hasCarrier();
};


/**
 * A container for a separated stereo signal.
 */
struct StereoSignal {
  bool hasPilot;
  Samples diff;
};


/**
 * A class to extract the stereo channel from a demodulated FM signal.
 */
class StereoSeparator {
  static const float kCorrThres;

  float sinTable_[8001];
  float cosTable_[8001];
  float sin_;
  float cos_;
  class ExpAverage;
  unique_ptr<ExpAverage> iavg_;
  unique_ptr<ExpAverage> qavg_;
  unique_ptr<ExpAverage> cavg_;

 public:
  /**
   * Constructor for the separator.
   * @param sampleRate The sample rate for the input signal.
   * @param pilotFreq The frequency of the pilot tone.
   */
  StereoSeparator(int sampleRate, int pilotFreq);
  ~StereoSeparator();

  /**
   * Locks on to the pilot tone and uses it to demodulate the stereo audio.
   * @param samples The original audio stream.
   * @return A container for the separated signal.
   */
  StereoSignal separate(const Samples& samples);
};


/**
 * A de-emphasis filter.
 */
class Deemphasizer {
  double mult_;
  double val_;

 public:
  /**
   * Constructor for the given sample rate and time constant.
   * @param sampleRate The signal's sample rate.
   * @param timeConstant_uS The filter's time constant in microseconds.
   */
  Deemphasizer(int sampleRate, int timeConstant_uS);

  /**
   * Deemphasizes the given samples in place.
   * @param samples The samples to deemphasize.
   */
  void inPlace(Samples& samples);
};


}  // namespace radioreceiver

#endif  // DSP_H_
