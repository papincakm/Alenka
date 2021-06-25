#include "tfanalyser.h"

#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "tfvisualizer.h"

#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>
//TODO: delete later
#include <iostream>
#include <string>


#include <sstream>


using namespace AlenkaFile;


TfAnalyser::TfAnalyser(QWidget *parent) : QWidget(parent) {
  connect(parent, SIGNAL(visibilityChanged(bool)),
    SLOT(parentVisibilityChanged(bool)));

  tfModel = std::make_unique<TfModel>();

  auto mainBox = new QVBoxLayout();
  auto menuBox = new QHBoxLayout();

  menuBox->addLayout(createChannelTimeMenu());
  menuBox->addWidget(createResolutionMenu());
  menuBox->addWidget(createFrequencyMenu());
  menuBox->addWidget(createFilterMenu());
  menuBox->addWidget(createFreezeMenu());

  mainBox->addLayout(menuBox);
  setupTfVisualizer(mainBox);
}

void TfAnalyser::setupTfVisualizer(QVBoxLayout* mainBox) {
  visualizer = new TfVisualizer(this);
  auto vizBox = new QVBoxLayout();
  vizBox->addWidget(visualizer);

  mainBox->addLayout(vizBox);

  setLayout(mainBox);

  //TODO: refactor this so it doesnt have to be set on multiple places
  visualizer->setSeconds(tfModel->secondsToDisplay);
}

void TfAnalyser::changeFile(OpenDataFile *file) {
		tfModel->file = file;
		if (file) {
				updateConnections();
				updateSpectrum();

        channelSpinBox->setRange(0, file->file->getChannelCount());

        tfModel->frequency = file->file->getSamplingFrequency() / 2;
        visualizer->setFrequency(tfModel->frequency);
        visualizer->setMinFrequency(0);
        visualizer->setMaxFrequency(tfModel->frequency);

        minFreqLine->clear();
        delete minFreqLine->validator();
        minFreqLine->insert(QString::number(0));
        minFreqLine->setValidator(new QIntValidator(0, tfModel->frequency, minFreqLine));

        maxFreqLine->clear();
        delete maxFreqLine->validator();
        maxFreqLine->insert(QString::number(tfModel->frequency));
        maxFreqLine->setValidator(new QIntValidator(tfModel->frequency / tfModel->freqBins + 1, tfModel->frequency,
          maxFreqLine));
		}
}

void TfAnalyser::updateConnections() {
		for (auto e : connections)
				disconnect(e);
		connections.clear();

		if (tfModel->file) {
				auto c = connect(&(tfModel->file)->infoTable, SIGNAL(positionChanged(int, double)), this,
						SLOT(updateSpectrum()));
				connections.push_back(c);
		}
}

bool TfAnalyser::ready() {
  if (!tfModel->file || tfModel->secondsToDisplay == 0 || !this->isVisible() || !parentVisible ||
    tfModel->frameSize <= 1 || tfModel->hopSize <= 0 || freeze || tfModel->frameSize < tfModel->hopSize)
    return false;
  return true;
}

void TfAnalyser::updateSpectrum() {
  if (!ready())
    return;

  std::vector<float> processedValues = tfModel->getStftValues();

  visualizer->setSeconds(tfModel->secondsToDisplay);
  visualizer->setDataToDraw(processedValues, tfModel->frameCount, tfModel->freqBinsUsed);
  visualizer->update();
}

void TfAnalyser::setFrameSize() {
  tfModel->frameSize = frameLine->text().toInt();

  hopLine->setValidator(new QIntValidator(MIN_HOP_SIZE, tfModel->frameSize + 1, hopLine));
  tfModel->maxFreqBinDraw = std::min(tfModel->maxFreqBinDraw, tfModel->freqBins);
  tfModel->minFreqBinDraw = std::min(tfModel->minFreqBinDraw, tfModel->freqBins);

  updateSpectrum();
}

void TfAnalyser::setHopSize() {
  tfModel->hopSize = hopLine->text().toInt();
  updateSpectrum();
}

void TfAnalyser::setMinFreqDraw() {
  if (minFreqLine->text().toInt() > maxFreqLine->text().toInt()) {
    minFreqLine->setText(QString::number(visualizer->getMinFrequency()));
    return;
  }

  visualizer->setMinFrequency(minFreqLine->text().toInt());
  float minFreqRatio = minFreqLine->text().toFloat() / tfModel->frequency;
  //QtValidator doesn't take care of values below Hz size of 1 freqBin
  tfModel->minFreqBinDraw = std::max(1, static_cast<int>(minFreqRatio * tfModel->freqBins));

  updateSpectrum();
}

void TfAnalyser::setMaxFreqDraw() {
  if (maxFreqLine->text().toInt() < minFreqLine->text().toInt()) {
    maxFreqLine->setText(QString::number(visualizer->getMaxFrequency()));
    return;
  }
  visualizer->setMaxFrequency(maxFreqLine->text().toInt());
  float maxFreqRatio = maxFreqLine->text().toFloat() / tfModel->frequency;

  //QtValidator doesn't take care of values below Hz size of 1 freqBin
  tfModel->maxFreqBinDraw = std::max(1, static_cast<int>(std::ceil(maxFreqRatio * tfModel->freqBins)));
  updateSpectrum();
}

void TfAnalyser::setChannelToDisplay(int ch) {
  tfModel->channelToDisplay = ch;
  updateSpectrum();
}

void TfAnalyser::setSecondsToDisplay(int s) {
  tfModel->secondsToDisplay = s;
  updateSpectrum();
}

void TfAnalyser::setFreezeSpectrum(bool f) {
  freeze = f;
  if (!f)
    updateSpectrum();
}

void TfAnalyser::setFCompensation(bool f) {
  tfModel->fCompensation = f;
  updateSpectrum();
}

void TfAnalyser::setLogCompensation(bool l) {
  tfModel->logCompensation = l;
  updateSpectrum();
}


void TfAnalyser::setFilterWindow(int wf) {
  tfModel->filterWindow = wf;
  updateSpectrum();
}

void TfAnalyser::parentVisibilityChanged(bool vis) {
  parentVisible = vis;
}

QLayout* TfAnalyser::createChannelTimeMenu() {
  QGridLayout* gLayout = new QGridLayout();
  //setup first row - channel
  QLabel* chanLabel = new QLabel("Channel:");
  chanLabel->setToolTip("Index of the channel from the original file to be used as "
    "input for the spectrum graph");

  channelSpinBox = new QSpinBox();
  //TODO: set max pos channel from file
  channelSpinBox->setRange(0, 0);
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
  timeSpinBox->setValue(tfModel->secondsToDisplay);

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
  frameLine->setValidator(new QIntValidator(MIN_FRAME_SIZE, MAX_FRAME_SIZE, frameLine));
  connect(frameLine, SIGNAL(editingFinished()), this,
    SLOT(setFrameSize()));
  frameLine->insert(QString::number(tfModel->frameSize));

  QLabel* frameSampleLabel = new QLabel("samples");

  gLayout->addWidget(frameLabel, 0, 0, 1, 1);
  gLayout->addWidget(frameLine, 0, 1, 1, 1);
  gLayout->addWidget(frameSampleLabel, 0, 2, 1, 1);

  //setup second row - hop size
  QLabel *hopLabel = new QLabel("Hop:  ", this);
  hopLabel->setToolTip("Gap between successive short-time fourier transforms.");

  hopLine = new QLineEdit();
  hopLine->setValidator(new QIntValidator(MIN_HOP_SIZE, tfModel->frameSize + 1, hopLine));
  connect(hopLine, SIGNAL(editingFinished()), this,
    SLOT(setHopSize()));
  hopLine->insert(QString::number(tfModel->hopSize));

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
  minFreqLine->setValidator(new QIntValidator(0, tfModel->frequency, minFreqLine));
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
  maxFreqLine->setValidator(new QIntValidator(tfModel->frequency / tfModel->freqBins + 1, tfModel->frequency,
    maxFreqLine));
  connect(maxFreqLine, SIGNAL(editingFinished()), this,
    SLOT(setMaxFreqDraw()));
  maxFreqLine->insert(QString::number(tfModel->frequency));

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
  windowCombo->setCurrentIndex(tfModel->filterWindow);
  fWindowBox->addWidget(windowCombo);
  filterBox->addLayout(fWindowBox);

  auto compBox = new QHBoxLayout();

  //1/f compensation
  QCheckBox *fCheckBox = new QCheckBox("1/f");
  fCheckBox->setToolTip("1/f compensation. Multiply every power value by its bin frequency.");
  connect(fCheckBox, SIGNAL(clicked(bool)), this,
    SLOT(setFCompensation(bool)));
  fCheckBox->setChecked(tfModel->fCompensation);
  compBox->addWidget(fCheckBox);

  //log compensation
  QCheckBox *logCheckBox = new QCheckBox("log");
  logCheckBox->setToolTip("Log compensation. Logarithmize(natural) every power value.");
  connect(logCheckBox, SIGNAL(clicked(bool)), this,
    SLOT(setLogCompensation(bool)));
  logCheckBox->setChecked(tfModel->logCompensation);
  compBox->addWidget(logCheckBox);

  filterBox->addLayout(compBox);

  filterGroup->setLayout(filterBox);

  return filterGroup;
}

QWidget* TfAnalyser::createFreezeMenu() {
  QCheckBox *checkBox = new QCheckBox("freeze");
  checkBox->setToolTip("Freeze STFT.");
  connect(checkBox, SIGNAL(clicked(bool)), this,
    SLOT(setFreezeSpectrum(bool)));
  checkBox->setChecked(freeze);
  return checkBox;
}