#ifndef TFMODEL_H
#define TFMODEL_H

#include "../../Alenka-Signal/include/AlenkaSignal/fftprocessor.h"
#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"

#include <vector>

class OpenDataFile;

//TODO: copied from filterprocessor, this should be in separate filter function file
template <class T> T hammingWindow(int n, int M) {
  const double tmp = 2 * M_PI * n / (M - 1);
  return static_cast<T>(0.54 - 0.46 * cos(tmp));
}

template <class T> T blackmanWindow(int n, int M) {
  const double a = 0.16, a0 = (1 - a) / 2, a1 = 0.5, a2 = a / 2,
    tmp = 2 * M_PI * n / (M - 1);
  return static_cast<T>(a0 - a1 * cos(tmp) + a2 * cos(2 * tmp));
}

namespace AlenkaSignal {
  class FftProcessor;
} // namespace AlenkaSignal

/**
* @brief Time frequency model.
*/
class TfModel {

  //fft processor related
  int parallelQueues = 0;
  std::unique_ptr<AlenkaSignal::FftProcessor> fftProcessor;

public:
  OpenDataFile* file = nullptr;
  int channelToDisplay = 0;
  int secondsToDisplay = 10;
  int frameSize = 128;
  int hopSize = 16;
  int filterWindow = 1;
  bool fCompensation = false;
  bool logCompensation = false;
  int minFreqBinDraw = 0;
  int maxFreqBinDraw = 65;
  int frequency = 0;
  int freqBins = 65;

  int frameCount = 0;
  int freqBinsUsed = 0;

	TfModel() {};

  /**
  * @brief Returns STFT result in a form of framcount * freqbin array.
  * Results are filtered by min/max freq filter.
  */
  std::vector<float> getStftValues();

private:
  void applyWindowFunction(std::vector<float>& data);
  std::vector<float> loadSamples();
};

#endif // TFMODEL_H
