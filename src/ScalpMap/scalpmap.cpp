#include "scalpmap.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "../options.h"
#include "../myapplication.h"
#include "scalpcanvas.h"
#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>
#include <cmath>

#include <QtWidgets>

using namespace AlenkaFile;
using namespace std;


ScalpMap::ScalpMap(QWidget *parent) : QWidget(parent) {
  connect(parent, SIGNAL(visibilityChanged(bool)),
    SLOT(parentVisibilityChanged(bool)));
  printTiming = isProgramOptionSet("printTiming");
  setupCanvas();
}

const AbstractTrackTable* getTrackTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->trackTable(
    OpenDataFile::infoTable.getSelectedMontage());
}

void ScalpMap::reset() {
  posValidCnt = 0;
  allowedPositions.clear();
}

void ScalpMap::changeFile(OpenDataFile *file) {
  this->file = file;
  if (file) {
    reset();

    model.useStereographicProjection = file->infoTable.getScalpMapProjection();

    updateFileInfoConnections();
    updateTrackTableConnections();
    updateLabels();
  }
}

bool ScalpMap::enabled() {
  return (isVisible() && parentVisible && posValidCnt >= 3);
}

void ScalpMap::deleteFileInfoConnections() {
  for (auto e : fileInfoConnections)
    disconnect(e);
  fileInfoConnections.clear();
}

void ScalpMap::deleteTrackTableConnections() {
  for (auto e : trackTableConnections)
    disconnect(e);
  trackTableConnections.clear();
}

void ScalpMap::updateFileInfoConnections() {
  deleteFileInfoConnections();

  auto c = connect(&file->infoTable, SIGNAL(signalCurPosProcessedChanged()),
    this, SLOT(updateSpectrum()));
  fileInfoConnections.push_back(c);

  c = connect(&file->infoTable, SIGNAL(useExtremaCustom()),
    this, SLOT(updateToExtremaCustom()));
  fileInfoConnections.push_back(c);

  c = connect(&file->infoTable, SIGNAL(useExtremaLocal()),
    this, SLOT(updateToExtremaLocal()));
  fileInfoConnections.push_back(c);

  c = connect(&file->infoTable, SIGNAL(scalpMapProjectionChanged(bool)),
    this, SLOT(setScalpMapProjection(bool)));
  fileInfoConnections.push_back(c);
}

void ScalpMap::parentVisibilityChanged(bool vis) {
  parentVisible = vis;
  if (vis)
    updateSpectrum();
}

void ScalpMap::updateTrackTableConnections() {
  if (!file)
    return;

  deleteTrackTableConnections();

  const AbstractTrackTable *trackTable = getTrackTable(file);

  if (0 < trackTable->rowCount()) {
    auto vitness = VitnessTrackTable::vitness(trackTable);

    auto c = connect(vitness, SIGNAL(valueChanged(int, int)), this, SLOT(updateLabels()));
    trackTableConnections.push_back(c);

    c = connect(vitness, SIGNAL(rowsInserted(int, int)), this, SLOT(updateLabels()));
    trackTableConnections.push_back(c);

    c = connect(vitness, SIGNAL(rowsRemoved(int, int)), this, SLOT(updateLabels()));
    trackTableConnections.push_back(c);
  }
}

bool ScalpMap::positionsValid(const std::vector<QVector3D>& positions) {
  if (positions.empty())
    return false;

  for (auto p : positions) {
    int same = 0;
    for (auto cp : positions) {
      if (cp == p)
        same++;
    }
    if (same > 1)
      return false;
  }

  return true;
}

void ScalpMap::setupCanvas() {
  auto box = new QVBoxLayout;
  setLayout(box);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);

  scalpCanvas = new ScalpCanvas(this);
  box->addWidget(scalpCanvas);
  setMinimumHeight(100);
  setMinimumWidth(100);

  // Set some OpenGL context details.
  QSurfaceFormat format;

  format.setVersion(2, 0);
  format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);

#ifndef NDEBUG
  format.setOption(QSurfaceFormat::DebugContext);
#endif

  scalpCanvas->setFormat(format);
}

void ScalpMap::updateLabels() {
  if (!file || file->dataModel->montageTable()->rowCount() <= 0)
    return;

  allowedPositions.clear();
  posValidCnt = 0;
  std::vector<QString> labels;
  std::vector<QVector3D> positions;

  const AbstractTrackTable *trackTable = getTrackTable(file);

  for (int i = 0; i < trackTable->rowCount(); i++) {
    Track t = trackTable->row(i);
    if (t.hidden)
      continue;

    if (std::isnan(t.x) || std::isnan(t.y) || std::isnan(t.z)) {
      allowedPositions.push_back(false);
      continue;
    }

    allowedPositions.push_back(true);
    posValidCnt++;

    labels.push_back(QString::fromStdString(t.label));
    positions.push_back(QVector3D(t.x, t.y, t.z));
  }

  if (!positionsValid(positions) || posValidCnt < 3) {
    scalpCanvas->forbidDraw("Electrode positions are invalid\n(Two positions can't have the same coordinates).\n"\
      "Change the coordinates or disable the invalid electrodes.");
    posValidCnt = 0;
    return;
  }

  std::vector<QVector2D> positionsProjected;
  positionsProjected = model.getPositionsProjected(positions);

  if (positionsProjected.empty()) {
    scalpCanvas->forbidDraw("Electrode positions are invalid\n(A sphere cant be fitted to the coordinates,"\
    "scalpmap cant be generated).\nChange the coordinates or disable the invalid electrodes.");
    posValidCnt = 0;
    return;
  }

  scalpCanvas->setChannelLabels(labels);
  scalpCanvas->setChannelPositions(positionsProjected);

  if (!enabled())
    return;

  scalpCanvas->allowDraw();
}

void ScalpMap::updateSpectrum() {
  if (!file || file->dataModel->montageTable()->rowCount() <= 0 || posValidCnt < 3)
    return;

  // benchmark
  decltype(chrono::high_resolution_clock::now()) start;
  if (printTiming) {  
    start = chrono::high_resolution_clock::now();
  }

  const AbstractTrackTable *trackTable =
    file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());

  if (trackTable->rowCount() <= 0)
    return;

  const int position = OpenDataFile::infoTable.getPosition();

  std::vector<float> samplesUnfiltered = OpenDataFile::infoTable.getSignalSampleCurPosProcessed();
  std::vector<float> samples;

  for (int i = 0; i < samplesUnfiltered.size(); i++) {
    if (allowedPositions[i])
      samples.push_back(samplesUnfiltered[i]);
  }

  if (OpenDataFile::infoTable.getScalpMapExtrema() == InfoTable::Extrema::local) {
    voltageMin = *std::min_element(std::begin(samples), std::end(samples));
    voltageMax = *std::max_element(std::begin(samples), std::end(samples));
  }

  scalpCanvas->setPositionVoltages(samples, voltageMin, voltageMax);

  if (printTiming) {
    const std::chrono::nanoseconds time = std::chrono::high_resolution_clock::now() - start;
    currentBenchTimeGlobal += time;
  }

  if (!enabled())
    return;

  scalpCanvas->allowDraw();
  scalpCanvas->update();
}

void ScalpMap::updateToExtremaLocal() {
  updateSpectrum();
}

void ScalpMap::updateToExtremaCustom() {
  QDialog* dialog = new QDialog(this, Qt::MSWindowsFixedSizeDialogHint);
  QFormLayout* form = new QFormLayout(dialog);

  QLabel* statusBar = new QLabel("Insert custom extrema values.", dialog);
  form->addRow(statusBar);

  //setup first row - min voltage
  QLabel* minLabel = new QLabel("Min:", this);
  minLabel->setToolTip("Minimum amplitude.");

  QLineEdit* minVolLine = new QLineEdit();
  minVolLine->setValidator(new QDoubleValidator(minVolLine));
  minVolLine->insert(QString::number(voltageMin));
  form->addRow(minLabel, minVolLine);

  //setup second row - max voltage
  QLabel* maxLabel = new QLabel("Max:  ", this);
  maxLabel->setToolTip("Maximum amplitude.");

  QLineEdit* maxVolLine = new QLineEdit();
  maxVolLine->setValidator(new QDoubleValidator(maxVolLine));
  maxVolLine->insert(QString::number(voltageMax));
  form->addRow(maxLabel, maxVolLine);

  // Add some standard buttons (Cancel/Ok) at the bottom of the dialog
  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
    Qt::Horizontal, dialog);
  form->addRow(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, [this, dialog, minVolLine, maxVolLine, statusBar]() {
    if (minVolLine->text().toFloat() > maxVolLine->text().toFloat()) {
      statusBar->setText("Min value has to be lesser than max value.");
      return;
    }

    voltageMin = minVolLine->text().toFloat();
    voltageMax = maxVolLine->text().toFloat();
    dialog->close();
    updateSpectrum();
  });

  connect(buttonBox, &QDialogButtonBox::rejected, [this, dialog]() {
    OpenDataFile::infoTable.setExtremaLocal();
    dialog->close();
  });

  dialog->exec();
}

void ScalpMap::setScalpMapProjection(bool proj) {
  model.useStereographicProjection = proj;
  updateLabels();
  updateSpectrum();
}
