#include "tfanalyser.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "../tfvisualizer.h"
#include "../myapplication.h"
#include "../options.h"

#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>
//TODO: delete later
#include <iostream>
#include <string>


#include <sstream>


using namespace AlenkaFile;


TfAnalyser::TfAnalyser(QWidget *parent) : QWidget(parent), fft(new Eigen::FFT<float>()) {
  connect(parent, SIGNAL(visibilityChanged(bool)),
    SLOT(parentVisibilityChanged(bool)));

  auto mainBox = new QVBoxLayout();
  //auto splitter = new QSplitter(Qt::Vertical);
  auto menuBox = new QHBoxLayout();

  menuBox->addLayout(createChannelTimeMenu());
  menuBox->addWidget(createResolutionMenu());
  menuBox->addWidget(createFrequencyMenu());
  menuBox->addWidget(createFilterMenu());

  //freeze select
  QCheckBox *checkBox = new QCheckBox("freeze");
  checkBox->setToolTip("Freeze STFT.");
  connect(checkBox, SIGNAL(clicked(bool)), this,
    SLOT(setFreezeSpectrum(bool)));
  checkBox->setChecked(freeze);
  menuBox->addWidget(checkBox);

  mainBox->addLayout(menuBox);
  setupTfVisualizer(mainBox);
}

QLayout* TfAnalyser::createChannelTimeMenu() {
  QGridLayout* gLayout = new QGridLayout();
  //setup first row - channel
  QLabel* chanLabel = new QLabel("Channel:");
  chanLabel->setToolTip("Index of the channel from the original file to be used as "
    "input for the spectrum graph");

  channelSpinBox = new QSpinBox();
  //TODO: set max pos channel from file
  channelSpinBox->setRange(0, 180);
  connect(channelSpinBox, SIGNAL(valueChanged(int)), this,
    SLOT(setChannelToDisplay(int)));

  gLayout->addWidget(chanLabel, 0, 0, 1, 1);
  gLayout->addWidget(channelSpinBox, 0, 1, 1, 1);

  //setup time
  QLabel* timeLabel = new QLabel("Time:");
  timeLabel->setToolTip("Time window for time-frequency analysis.");

  auto timeSpinBox = new QSpinBox();
  //TODO: set max time from what?
  timeSpinBox->setRange(0, 100);
  connect(timeSpinBox, SIGNAL(valueChanged(int)), this,
    SLOT(setSecondsToDisplay(int)));
  timeSpinBox->setValue(secondsToDisplay);

  QLabel* secLabel = new QLabel("sec");
  secLabel->setToolTip("Interval in seconds to be used for the STFT");

  gLayout->addWidget(timeLabel, 1, 0, 1, 1);
  gLayout->addWidget(timeSpinBox, 1, 1, 1, 1);
  gLayout->addWidget(secLabel, 1, 2, 1, 1);

  return gLayout;
}

QGroupBox* TfAnalyser::createResolutionMenu() {
  QGroupBox* resGroup = new QGroupBox(tr("Resolution"));

  QGridLayout* gLayout = new QGridLayout();

  //setup first row - frame size
  QLabel* frameLabel = new QLabel("Frame:", this);
  frameLabel->setToolTip("Size of single short-time fourier transform.");

  frameLine = new QLineEdit();
  //TODO: what should be the max value here?
  frameLine->setValidator(new QIntValidator(0, 2048, frameLine));
  connect(frameLine, SIGNAL(editingFinished()), this,
    SLOT(setFrameSize()));
  frameLine->insert(QString::number(frameSize));

  QLabel* frameSampleLabel = new QLabel("samples");

  gLayout->addWidget(frameLabel, 0, 0, 1, 1);
  gLayout->addWidget(frameLine, 0, 1, 1, 1);
  gLayout->addWidget(frameSampleLabel, 0, 2, 1, 1);

  //setup second row - hop size
  QLabel *hopLabel = new QLabel("Hop:  ", this);
  hopLabel->setToolTip("Gap between successive short-time fourier transforms.");

  hopLine = new QLineEdit();
  hopLine->setValidator(new QIntValidator(0, frameSize + 1, hopLine));
  connect(hopLine, SIGNAL(editingFinished()), this,
    SLOT(setHopSize()));
  hopLine->insert(QString::number(hopSize));

  QLabel* hopSampleLabel = new QLabel("samples");

  gLayout->addWidget(hopLabel, 1, 0, 1, 1);
  gLayout->addWidget(hopLine, 1, 1, 1, 1);
  gLayout->addWidget(hopSampleLabel, 1, 2, 1, 1);

  resGroup->setLayout(gLayout);

  return resGroup;
}

QGroupBox* TfAnalyser::createFrequencyMenu() {
  QGroupBox* freqGroup = new QGroupBox(tr("Frequency"));

  QGridLayout* gLayout = new QGridLayout();

  //setup first row - frame size
  QLabel* minLabel = new QLabel("Min:", this);
  minLabel->setToolTip("Minimum frequency for spectogram.");

  minFreqLine = new QLineEdit();
  minFreqLine->setValidator(new QIntValidator(0, frequency, minFreqLine));
  connect(minFreqLine, SIGNAL(editingFinished()), this,
    SLOT(setMinFreqDraw()));
  minFreqLine->insert(QString::number(0));

  QLabel* minHzLabel = new QLabel("Hz");

  gLayout->addWidget(minLabel, 0, 0, 1, 1);
  gLayout->addWidget(minFreqLine, 0, 1, 1, 1);
  gLayout->addWidget(minHzLabel, 0, 2, 1, 1);

  //setup second row - hop size
  QLabel *maxLabel = new QLabel("Max:  ", this);
  maxLabel->setToolTip("Maximum frequency for spectogram.");

  maxFreqLine = new QLineEdit();
  maxFreqLine->setValidator(new QIntValidator(frequency / freqBins + 1, frequency, maxFreqLine));
  connect(maxFreqLine, SIGNAL(editingFinished()), this,
    SLOT(setMaxFreqDraw()));
  maxFreqLine->insert(QString::number(0));

  QLabel* maxHzLabel = new QLabel("Hz");

  gLayout->addWidget(maxLabel, 1, 0, 1, 1);
  gLayout->addWidget(maxFreqLine, 1, 1, 1, 1);
  gLayout->addWidget(maxHzLabel, 1, 2, 1, 1);

  freqGroup->setLayout(gLayout);

  return freqGroup;
}

QGroupBox* TfAnalyser::createFilterMenu() {
  QGroupBox* filterGroup = new QGroupBox(tr("Filter"));
  auto filterBox = new QVBoxLayout();
  auto fWindowBox = new QHBoxLayout();

  QLabel* FilterWindowlabel = new QLabel("Window:");
  FilterWindowlabel->setToolTip(
    "Window function to be used to filter data going to the STFT.");
  fWindowBox->addWidget(FilterWindowlabel);

  auto windowCombo = new QComboBox();
  //0
  windowCombo->addItem("None");
  //1
  windowCombo->addItem("Hamming");
  //2
  windowCombo->addItem("Blackman");
  connect(windowCombo, SIGNAL(currentIndexChanged(int)), this,
    SLOT(setFilterWindow(int)));
  windowCombo->setCurrentIndex(filterWindow);
  fWindowBox->addWidget(windowCombo);
  filterBox->addLayout(fWindowBox);

  auto compBox = new QHBoxLayout();

  //1/f compensation
  QCheckBox *fCheckBox = new QCheckBox("1/f");
  fCheckBox->setToolTip("1/f compensation. Multiply every power value by its bin frequency.");
  connect(fCheckBox, SIGNAL(clicked(bool)), this,
    SLOT(setFCompensation(bool)));
  fCheckBox->setChecked(fCompensation);
  compBox->addWidget(fCheckBox);

  //log compensation
  QCheckBox *logCheckBox = new QCheckBox("log");
  logCheckBox->setToolTip("Log compensation. Logarithmize(natural) every power value.");
  connect(logCheckBox, SIGNAL(clicked(bool)), this,
    SLOT(setLogCompensation(bool)));
  logCheckBox->setChecked(logCompensation);
  compBox->addWidget(logCheckBox);

  filterBox->addLayout(compBox);

  filterGroup->setLayout(filterBox);

  return filterGroup;
}

void TfAnalyser::setupTfVisualizer(QVBoxLayout* mainBox) {
  visualizer = new TfVisualizer(this);
  auto vizBox = new QVBoxLayout();
  vizBox->addWidget(visualizer);

  mainBox->addLayout(vizBox);

  setLayout(mainBox);

  //TODO: refactor this so it doesnt have to be set on multiple places
  visualizer->setSeconds(secondsToDisplay);
}

void TfAnalyser::changeFile(OpenDataFile *file) {
		this->file = file;
		if (file) {
				updateConnections();
				updateSpectrum();

        frequency = file->file->getSamplingFrequency() / 2;
        visualizer->setFrequency(file->file->getSamplingFrequency() / 2);

        minFreqLine->clear();
        delete minFreqLine->validator();
        minFreqLine->insert(QString::number(0));
        minFreqLine->setValidator(new QIntValidator(0, frequency, minFreqLine));

        maxFreqLine->clear();
        delete maxFreqLine->validator();
        maxFreqLine->insert(QString::number(frequency));
        maxFreqLine->setValidator(new QIntValidator(frequency / freqBins + 1, frequency, maxFreqLine));
        std::cout << "maxFreqVal: " << frequency / freqBins << "\n";
		}
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void TfAnalyser::updateConnections() {
		for (auto e : connections)
				disconnect(e);
		connections.clear();

		if (file) {
				auto c = connect(&file->infoTable, SIGNAL(positionChanged(int, double)), this,
						SLOT(updateSpectrum()));
				connections.push_back(c);
		}
}

bool TfAnalyser::ready() {
  //TODO: widget is visible when tabbed into different widget
  if (!file || secondsToDisplay == 0 || !this->isVisible() || !parentVisible || frameSize <= 0 || hopSize <= 0 ||
      freeze)
    return false;
  return true;
}

void TfAnalyser::applyWindowFunction(std::vector<float>& data) {
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

//TODO: copied from filtervisualizer
//TODO: make parent class for filter and this that has signal file manipulation methods and menus
void TfAnalyser::updateSpectrum() {
  if (!ready())
    return;

  //fft processor
  if (!fftProcessor) {
    programOption("parProc", parallelQueues);
    fftProcessor = std::make_unique<AlenkaSignal::FftProcessor>(globalContext.get(), parallelQueues);
  }

  assert(channelToDisplay < static_cast<int>(file->file->getChannelCount()));

  const int samplesToUse =
    secondsToDisplay * static_cast<int>(file->file->getSamplingFrequency());

  int frameCount = (samplesToUse - frameSize) / hopSize + 1;

  //TODO: check if this should be possible, right now it throws error
  if (frameCount < 1 || frameSize < hopSize)
    return;

  const int totalSampleCount = frameCount * frameSize;  
  const int position = OpenDataFile::infoTable.getPosition();
  int startFileSample = std::max(0, position - samplesToUse / 2);
  int endFileSample = std::min(static_cast<int>(file->file->getSamplesRecorded()), startFileSample + samplesToUse - 1);

  //read data
  int readSampleCount = endFileSample - startFileSample + 2; // TODO: Maybe round this to a power of two.
  std::vector<float> buffer(readSampleCount * file->file->getChannelCount());
  file->file->readSignal(buffer.data(), startFileSample, endFileSample);

  auto begin = buffer.begin() + readSampleCount * channelToDisplay;
  std::vector<float> signal(begin, begin + readSampleCount);

  for (int i = readSampleCount; i < totalSampleCount; i++) {
    signal.push_back(0.0f);
  }

  std::vector<float> fftValues;
  int tempFrameSize;

  std::vector<std::complex<float>> spectrumOld;
  for (int f = 0; f < frameCount; f++) {
    auto begin = signal.begin() + f * hopSize;
    std::vector<float> input(begin, begin + frameSize);

    //TODO: mby do this in fftprocessor
    applyWindowFunction(input);

    //is power of 2
    //TODO: is this correct?
    tempFrameSize = frameSize;
    while ((tempFrameSize  & (tempFrameSize - 1)) != 0) {
      input.push_back(0.0f);
      tempFrameSize++;
    }

    fftValues.insert(fftValues.end(), input.begin(), input.end());
  }

  std::vector<std::complex<float>> spectrum;
  spectrum = fftProcessor->process(fftValues, globalContext.get(), frameCount, tempFrameSize);
  std::vector<float> processedValues;

  int freqBinsUsed = maxFreqBinDraw - minFreqBinDraw;
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

  visualizer->setSeconds(secondsToDisplay);
  visualizer->setDataToDraw(processedValues, frameCount, freqBinsUsed);
  visualizer->update();
}

void TfAnalyser::setFrameSize() {
  frameSize = frameLine->text().toInt();
  freqBins = frameSize / 2 + 1;
  maxFreqBinDraw = std::min(maxFreqBinDraw, freqBins);
  minFreqBinDraw = std::min(minFreqBinDraw, freqBins);

  updateSpectrum();
}

void TfAnalyser::setHopSize() {
  hopSize = hopLine->text().toInt();
  updateSpectrum();
}

void TfAnalyser::setMinFreqDraw() {
  visualizer->setMinFrequency(minFreqLine->text().toInt());
  float minFreqRatio = minFreqLine->text().toFloat() / frequency;
  //QtValidator doesn't take care of values below Hz size of 1 freqBin
  minFreqBinDraw = std::max(1, static_cast<int>(minFreqRatio * freqBins));

  updateSpectrum();
}

void TfAnalyser::setMaxFreqDraw() {
  visualizer->setMaxFrequency(maxFreqLine->text().toInt());
  float maxFreqRatio = maxFreqLine->text().toFloat() / frequency;
  //QtValidator doesn't take care of values below Hz size of 1 freqBin
  maxFreqBinDraw = std::max(1, static_cast<int>(std::ceil(maxFreqRatio * freqBins)));
  updateSpectrum();
}

//TODO: rework to set by channel label
void TfAnalyser::setChannelToDisplay(int ch) {
  channelToDisplay = ch;
  updateSpectrum();
}

void TfAnalyser::setSecondsToDisplay(int s) {
  secondsToDisplay = s;
  updateSpectrum();
}

void TfAnalyser::setFreezeSpectrum(bool f) {
  freeze = f;
  if (!f)
    updateSpectrum();
}

void TfAnalyser::setFCompensation(bool f) {
  fCompensation = f;
  updateSpectrum();
}

void TfAnalyser::setLogCompensation(bool l) {
  logCompensation = l;
  updateSpectrum();
}


void TfAnalyser::setFilterWindow(int wf) {
  filterWindow = wf;
  updateSpectrum();
}

void TfAnalyser::parentVisibilityChanged(bool vis) {
  parentVisible = vis;
}