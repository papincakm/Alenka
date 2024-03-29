#include "tfmodel.h"
#include "../options.h"
#include "../myapplication.h"
#include "../DataModel/infotable.h"

#include <algorithm>

#ifndef __APPLE__
#if defined WIN_BUILD
#include <windows.h>
#elif defined UNIX_BUILD
#include <GL/glx.h>
#endif
#endif

using namespace std;
using namespace AlenkaFile;

std::vector<float> TfModel::getStftValues() {

  //fft processor
  if (!fftProcessor) {
    programOption("parProc", parallelQueues);
    fftProcessor = std::make_unique<AlenkaSignal::FftProcessor>(globalContext.get(), parallelQueues);
  }

  assert(channelToDisplay < static_cast<int>(file->file->getChannelCount()));

  std::vector<float> signal = loadSamples();

  std::vector<float> fftInput;
  int zeroPaddedFrameSize = 0;
  for (int f = 0; f < frameCount; f++) {
    auto begin = signal.begin() + f * hopSize;
    std::vector<float> frameSamples(begin, begin + frameSize);

    applyWindowFunction(frameSamples);

    //pad with zeroes
    zeroPaddedFrameSize = frameSize;
    while ((zeroPaddedFrameSize & (zeroPaddedFrameSize - 1)) != 0) {
      frameSamples.push_back(0.0f);
      zeroPaddedFrameSize++;
    }

    fftInput.insert(fftInput.end(), std::make_move_iterator(frameSamples.begin()),
      std::make_move_iterator(frameSamples.end()));
  }

  std::vector<std::complex<float>> spectrum = fftProcessor->process(fftInput, globalContext.get(),
    frameCount, zeroPaddedFrameSize);

  freqBins = zeroPaddedFrameSize / 2 + 1;

  //get magnitudes and filter undesired frequencies
  std::vector<float> processedValues;
  freqBinsUsed = maxFreqBinDraw - minFreqBinDraw;
  for (int fc = 0; fc < frameCount; fc++) {
    for (int fb = minFreqBinDraw; fb < maxFreqBinDraw; fb++) {
      float val = std::abs(spectrum[fc * freqBins + fb]);
      if (std::isfinite(val)) {

        if (fCompensation) {
          val *= (fb + 1) * static_cast<float>(frequency) / freqBins;
        }

        if (logCompensation) {
          val = std::log(val);
        }

        processedValues.push_back(val);
      }
      else {
        processedValues.push_back(0);
      }
    }
  }

  return processedValues;
}

void TfModel::applyWindowFunction(std::vector<float>& data) {
  switch (filterWindow) {
  case 0:
    return;
  case 1:
    for (size_t i = 0; i < data.size(); i++) {
      data[i] = hammingWindow<float>(data[i], data.size());
    }
    break;
  case 2:
    for (size_t i = 0; i < data.size(); i++) {
      data[i] = blackmanWindow<float>(data[i], data.size());
    }
    break;
  }
}

std::vector<float> TfModel::loadSamples() {
  int samplesToUse =
    secondsToDisplay * static_cast<int>(file->file->getSamplingFrequency());

  frameCount = (samplesToUse - frameSize) / hopSize + 1;
  samplesToUse = frameSize * frameCount;

  const int position = OpenDataFile::infoTable.getPosition();

  int startFileSample = max(0, position - samplesToUse / 2);

  int endFileSample = min(static_cast<int>(file->file->getSamplesRecorded()), position + samplesToUse / 2);

  //read data
  int readSampleCount = endFileSample - startFileSample + 2;
  std::vector<float> buffer(readSampleCount * file->file->getChannelCount());
  file->file->readSignal(buffer.data(), startFileSample, endFileSample);

  auto begin = buffer.begin() + readSampleCount * channelToDisplay;
  std::vector<float> signal;

  //pad with zeroes at the beginning
  for (int i = position - samplesToUse / 2; i <= 0; i++) {
    signal.push_back(0.0f);
  }

  signal.insert(signal.end(), begin, begin + readSampleCount);

  //pad with zeroes at the end
  const int totalSampleCount = frameCount * hopSize + frameSize;
  for (int i = readSampleCount; i < totalSampleCount; i++) {
    signal.push_back(0.0f);
  }

  return signal;
}