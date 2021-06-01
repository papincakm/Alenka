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
  auto mainBox = new QVBoxLayout();
  //auto splitter = new QSplitter(Qt::Vertical);

  //setupMenu
  auto menuBox = new QHBoxLayout();

  //frame size
  QLabel* frameLabel = new QLabel("Frame:", this);
  frameLabel->setToolTip("Single frame size, in samples, for short-time fourier transform.");
  menuBox->addWidget(frameLabel);

  frameLine = new QLineEdit();
  frameLine->setValidator(new QIntValidator(frameLine));
  connect(frameLine, SIGNAL(editingFinished()), this,
    SLOT(setFrameSize()));
  frameLine->insert(QString::number(frameSize));

  menuBox->addWidget(frameLine);

  //hop size

  QLabel *hopLabel = new QLabel("Hop:", this);
  hopLabel->setToolTip("Hop size, in samples, between successive DFTs.");
  menuBox->addWidget(hopLabel);

  hopLine = new QLineEdit();
  hopLine->setValidator(new QIntValidator(hopLine));
  connect(hopLine, SIGNAL(editingFinished()), this,
    SLOT(setHopSize()));
  hopLine->insert(QString::number(hopSize));

  menuBox->addWidget(hopLine);

  //channel select
  QLabel* chanLabel = new QLabel("Channel");
  chanLabel->setToolTip("Index of the channel from the original file to be used as "
    "input for the spectrum graph");
  menuBox->addWidget(chanLabel);

  channelSpinBox = new QSpinBox();
  channelSpinBox->setRange(0, 180);
  connect(channelSpinBox, SIGNAL(valueChanged(int)), this,
    SLOT(setChannelToDisplay(int)));

  menuBox->addWidget(channelSpinBox);

  //time select
  auto spinBox = new QSpinBox();
  spinBox->setRange(0, 100);
  connect(spinBox, SIGNAL(valueChanged(int)), this,
    SLOT(setSecondsToDisplay(int)));

  spinBox->setValue(secondsToDisplay);
  menuBox->addWidget(spinBox);

  QLabel* secLabel = new QLabel("sec");
  secLabel->setToolTip("Interval in seconds to be used for the STFT");
  menuBox->addWidget(secLabel);

  //window function select
  QLabel* FilterWindowlabel = new QLabel("Filter window:");
  FilterWindowlabel->setToolTip(
    "Window function to be used to modify data going to the STFT.");
  menuBox->addWidget(FilterWindowlabel);

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

  menuBox->addWidget(windowCombo);

  //freeze select
  QCheckBox *checkBox = new QCheckBox("freeze");
  checkBox->setToolTip("Freeze STFT.");
  connect(checkBox, SIGNAL(clicked(bool)), this,
    SLOT(setFreezeSpectrum(bool)));
  checkBox->setChecked(freeze);
  menuBox->addWidget(checkBox);

  //setupVisualizer
  visualizer = new TfVisualizer(this);
  auto vizBox = new QVBoxLayout();
  vizBox->addWidget(visualizer);

  mainBox->addLayout(menuBox);
  mainBox->addLayout(vizBox);

  setLayout(mainBox);

  //TODO: refactor this so it doesnt have to be set on multiple places
  visualizer->setSeconds(secondsToDisplay);
  visualizer->setFrameSize(frameSize);
}

void TfAnalyser::changeFile(OpenDataFile *file) {
		this->file = file;
		if (file) {
				updateConnections();
				updateSpectrum();
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
  if (!file || secondsToDisplay == 0 || !this->isVisible() || frameSize <= 0 || hopSize <= 0 ||
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

  int freqBins = frameSize / 2 + 1;
  std::vector<float> fftValues;
  int tempFrameSize;

  std::vector<std::complex<float>> spectrumOld;
  for (int f = 0; f < frameCount; f++) {
    auto begin = signal.begin() + f * hopSize;
    std::vector<float> input(begin, begin + frameSize);

    applyWindowFunction(input);

    //is power of 2
    //TODO: is this correct?
    tempFrameSize = frameSize;
    while ((tempFrameSize  & (tempFrameSize - 1)) != 0) {
      input.push_back(0.0f);
      tempFrameSize++;
      //std::cout << "not power of 2\n";
    }


    fftValues.insert(fftValues.end(), input.begin(), input.end());
    //if (f == 0) {
      /*std::cout << "oldFFT size:" << spectrum.size() << "\n";
      int c = 0;
      for (auto s : spectrum) {
        if (c >= 65)
          break;
        std::cout << s << " ";
        c++;
      }
      std::cout << "\n";*/
    //}
    /*std::vector<std::complex<float>> spectrum;
    fft->fwd(spectrum, input);
    spectrumOld.insert(spectrumOld.end(), spectrum.begin(), spectrum.end());*/
  }

  //std::vector<std::complex<float>> spectrum;
  std::vector<std::complex<float>> spectrum;
  //TODO: do everything in one clfft batch
  spectrum = fftProcessor->process(fftValues, globalContext.get(), frameCount, tempFrameSize);

  /*for (int i = 0; i < spectrumOld.size(); i++) {
    std::cout << "\nold: " << std::abs(spectrumOld[i]) << " new:" << std::abs(spectrum[i]) << "\n";
    if (i % 65 == 0)
      std::cout << "\nNEXT\n";
    if (std::abs(std::abs(spectrumOld[i]) - std::abs(spectrum[i])) > 0.1f) {
      assert(1 == 2);
    }
  }*/

  /*std::cout << "spectrum, frameCount: " << frameCount << "\n";
  int c = 0;
  for (auto i : spectrum) {
    std::cout << i << " ";
    c++;
    if (c % 65 == 0)
      std::cout << "\nNEXT " << c / 65 << "\n";
  }*/

  std::vector<float> processedValues;

  for (int f = 0; f < spectrum.size(); f++) {
    //assert(static_cast<int>(input.size()) == tempFrameSize);
    //assert(static_cast<int>(spectrum.size()) == tempFrameSize);

    float val = std::abs(spectrum[f]);
    if (std::isfinite(val)) {
      processedValues.push_back(val);
    }
    else {
      processedValues.push_back(0);
    }
  }

  visualizer->setSeconds(secondsToDisplay);
  visualizer->setFrameSize(frameSize);
  visualizer->setDataToDraw(processedValues, frameCount, freqBins);
  visualizer->update();
}

void TfAnalyser::setFrameSize() {
  frameSize = frameLine->text().toInt();
  updateSpectrum();
}

void TfAnalyser::setHopSize() {
  hopSize = hopLine->text().toInt();
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

void TfAnalyser::setFilterWindow(int wf) {
  filterWindow = wf;
  updateSpectrum();
}