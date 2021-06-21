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

using namespace AlenkaFile;

std::vector<float> TfModel::getStftValues() {

  //fft processor
  if (!fftProcessor) {
    programOption("parProc", parallelQueues);
    fftProcessor = std::make_unique<AlenkaSignal::FftProcessor>(globalContext.get(), parallelQueues);
  }

  assert(channelToDisplay < static_cast<int>(file->file->getChannelCount()));

  const int samplesToUse =
    secondsToDisplay * static_cast<int>(file->file->getSamplingFrequency());

  frameCount = (samplesToUse - frameSize) / hopSize + 1;

  const int position = OpenDataFile::infoTable.getPosition();

  int startFileSample = max(0, position - samplesToUse / 2);
  //TODO: there was startFileSample + samplesToUse - 1, is it ok?
  int endFileSample = min(static_cast<int>(file->file->getSamplesRecorded()), position + samplesToUse / 2);

  //read data
  int readSampleCount = endFileSample - startFileSample + 2;
  std::vector<float> buffer(readSampleCount * file->file->getChannelCount());
  file->file->readSignal(buffer.data(), startFileSample, endFileSample);

  auto begin = buffer.begin() + readSampleCount * channelToDisplay;
  std::vector<float> signal;

  //pad with zeroes at beginning
  for (int i = position - samplesToUse / 2; i <= 0; i++) {
    signal.push_back(0.0f);
  }

  signal.insert(signal.end(), begin, begin + readSampleCount);

  //pad with zeroes on end
  const int totalSampleCount = frameCount * hopSize + frameSize;
  for (int i = readSampleCount; i < totalSampleCount; i++) {
    signal.push_back(0.0f);
  }

  std::vector<float> fftValues;

  int zeroPaddedFrameSize;
  for (int f = 0; f < frameCount; f++) {
    auto begin = signal.begin() + f * hopSize;
    std::vector<float> input(begin, begin + frameSize);

    //TODO: mby do this in fftprocessor
    applyWindowFunction(input);

    //pad with zeroes
    //TODO: is this correct?
    zeroPaddedFrameSize = frameSize;
    while ((zeroPaddedFrameSize  & (zeroPaddedFrameSize - 1)) != 0) {
      input.push_back(0.0f);
      zeroPaddedFrameSize++;
    }

    fftValues.insert(fftValues.end(), input.begin(), input.end());
  }

  std::vector<std::complex<float>> spectrum = fftProcessor->process(fftValues, globalContext.get(),
    frameCount, zeroPaddedFrameSize);
  std::vector<float> processedValues;
  
  freqBins = zeroPaddedFrameSize / 2 + 1;

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
    for (int i = 0; i < data.size(); i++) {
      data[i] = hammingWindow<float>(data[i], data.size());
    }
    break;
  case 2:
    for (int i = 0; i < data.size(); i++) {
      data[i] = blackmanWindow<float>(data[i], data.size());
    }
    break;
  }
}