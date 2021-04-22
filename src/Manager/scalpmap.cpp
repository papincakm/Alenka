#include "scalpmap.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "../scalpcanvas.h"
#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>
//TODO: delete later
#include <iostream>
#include <string>


#include <sstream>

#include <QtWidgets>

using namespace AlenkaFile;
using namespace std;


ScalpMap::ScalpMap(QWidget *parent) : QWidget(parent) {
  connect(&OpenDataFile::infoTable, SIGNAL(selectedMontageChanged(int)), this,
    SLOT(updateTrackTableConnections(int)));
  
  //std::cout << "SCALPMAP CONSTRUCTOR CALLED\n";
}

void ScalpMap::changeFile(OpenDataFile *file) {
  this->file = file;

  if (enabled()) {
    if (!scalpCanvas) {
      //std::cout << "change file SETUPING canvas\n";
      setupCanvas();
    }
    else {
      scalpCanvas->clear();
      updateLabels();
    }
  }
}

bool ScalpMap::enabled() {
  return (file && isVisible());
}

void ScalpMap::deleteScalpConnections() {
  for (auto e : scalpConnections)
    disconnect(e);
  scalpConnections.clear();
}

void ScalpMap::deleteTrackTableConnections() {
  for (auto e : trackTableConnections)
    disconnect(e);
  trackTableConnections.clear();
}

void ScalpMap::setupScalpConnections() {
  auto c = connect(&file->infoTable, SIGNAL(signalCurPosProcessedChanged()),
    this, SLOT(updateSpectrum()));
  scalpConnections.push_back(c);

  c = connect(&file->infoTable, SIGNAL(useExtremaGlobal()),
    this, SLOT(updateToExtremaGlobal()));
  scalpConnections.push_back(c);

  c = connect(&file->infoTable, SIGNAL(useExtremaLocal()),
    this, SLOT(updateToExtremaLocal()));
  scalpConnections.push_back(c);
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateTrackTableConnections(int row) {
  if (enabled()) {
    deleteTrackTableConnections();

    const AbstractMontageTable *mt = file->dataModel->montageTable();

    if (0 <= row && row < mt->rowCount()) {
      auto vitness = VitnessTrackTable::vitness(mt->trackTable(row));

      auto c = connect(vitness, SIGNAL(valueChanged(int, int)), this, SLOT(updateLabels()));
      trackTableConnections.push_back(c);

      c = connect(vitness, SIGNAL(rowsInserted(int, int)), this, SLOT(updateLabels()));
      trackTableConnections.push_back(c);

      c = connect(vitness, SIGNAL(rowsRemoved(int, int)), this, SLOT(updateLabels()));
      trackTableConnections.push_back(c);
    }
  }
}

//TODO: refactor
bool ScalpMap::positionsValid() {
  for (auto p : positions) {
    //std::cout << "POSITION x: " << p.x << "y: " << p.y << std::endl;
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

void ScalpMap::hideEvent(QHideEvent * event) {
  //std::cout << "hideEvent\n";
  if (scalpCanvas && scalpCanvas->isVisible()) {
    //scalpCanvas.reset();
    delete scalpCanvas;
  }

  QLayout* layout = this->layout();
  if (layout) {
    delete layout;
  }

  deleteScalpConnections();
}

void ScalpMap::setupCanvas() {
  //std::cout << "setuping canvas\n";
  auto box = new QVBoxLayout;
  setLayout(box);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);

  //scalpCanvas = make_unique<ScalpCanvas>(this);
  scalpCanvas = new ScalpCanvas(this);
  //box->addWidget(scalpCanvas.get());
  box->addWidget(scalpCanvas);
  setMinimumHeight(100);
  setMinimumWidth(100);

  setupScalpConnections();
  updateLabels();
}

void ScalpMap::showEvent(QShowEvent* event) {
  //udocking calls destructor in scalpCanvas
  //std::cout << "showEvent\n";
  if (enabled() || (scalpCanvas && scalpCanvas->isVisible()))
    setupCanvas();
}

//TODO: copied from canvas.cpp
const AbstractTrackTable* getTrackTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->trackTable(
    OpenDataFile::infoTable.getSelectedMontage());
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateLabels() {
  if (!enabled() || file->dataModel->montageTable()->rowCount() <= 0)
    return;
  
  labels.clear();
  colors.clear();
  positions.clear();

  const AbstractTrackTable *trackTable = getTrackTable(file);

  for (int i = 0; i < trackTable->rowCount(); i++) {
    Track t = trackTable->row(i);

    if (!t.hidden) {
      //TODO: labels, colors and positions should be in one class and one vector
      labels.push_back(QString::fromStdString(t.label));
      colors.push_back(DataModel::array2color<QColor>(t.color));
      positions.push_back(QVector3D(t.x, t.y, t.z));
    }
  }

  if (!positionsValid() || trackTable->rowCount() < 3) {
    scalpCanvas->forbidDraw("Channel positions are invalid(Two positions can't be the same).");
    return;
  }
  
  if (selectedExtrema == InfoTable::Extrema::global)
    updateExtremaGlobalValue();

  updatePositionsProjected();

  scalpCanvas->setChannelLabels(labels);
  scalpCanvas->setChannelPositions(positionsProjected);

  scalpCanvas->allowDraw();
}

void ScalpMap::setupExtrema() {
  switch (OpenDataFile::infoTable.getScalpMapExtrema()) {
  case InfoTable::Extrema::local:
    selectedExtrema = InfoTable::Extrema::local;
    break;
  case InfoTable::Extrema::global:
    selectedExtrema = InfoTable::Extrema::global;
    updateExtremaGlobalValue();
    break;
  case InfoTable::Extrema::custom:
    //TODO: do extrema custom
    //updateToExtremaLocal();
    selectedExtrema = InfoTable::Extrema::local;
    break;
  }
}

//TODO: investigate where freq are stored if canvas cant draw
void ScalpMap::updateSpectrum() {
  if (!enabled() || !scalpCanvas)
    return;

  //TODO: rethink this, extreme is not selected sometimes on first app run
  //if (selectedExtrema < InfoTable::Extrema::custom || selectedExtrema > InfoTable::Extrema::local)
    setupExtrema();

  const AbstractTrackTable *trackTable =
    file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());

  if (trackTable->rowCount() <= 0)
    return;

  const int position = OpenDataFile::infoTable.getPosition();

  std::vector<float> signalSamplePosition = OpenDataFile::infoTable.getSignalSampleCurPosProcessed();

  if (selectedExtrema == InfoTable::Extrema::local) {
    frequencyMin = *std::min_element(std::begin(signalSamplePosition), std::end(signalSamplePosition));
    frequencyMax = *std::max_element(std::begin(signalSamplePosition), std::end(signalSamplePosition));
  }

  scalpCanvas->setPositionFrequencies(signalSamplePosition, frequencyMin, frequencyMax);
  scalpCanvas->update();
}

void ScalpMap::updateExtremaGlobalValue() {
  //get hidden channels vector
  std::vector<bool> hidden;
  const AbstractTrackTable *trackTable = getTrackTable(file);
  for (int i = 0; i < trackTable->rowCount(); i++) {
    Track t = trackTable->row(i);
    hidden.push_back(t.hidden);
  }

  frequencyMin = file->file->getGlobalPhysicalMinimum(hidden);
  frequencyMax = file->file->getGlobalPhysicalMaximum(hidden);
}

void ScalpMap::updateToExtremaGlobal() {
  selectedExtrema = InfoTable::Extrema::global;
  updateExtremaGlobalValue();
  updateSpectrum();
}

void ScalpMap::updateToExtremaLocal() {
  selectedExtrema = InfoTable::Extrema::local;
  updateSpectrum();
}

float degToRad(float n) {
  return n * M_PI / 180;
}

float radToDeg(float n) {
  return n * 180 / M_PI;
}

//TODO: improvised algorithm, find a better way to do this
void ScalpMap::updatePositionsProjected() {
  positionsProjected.clear();

  //compute radius of sphere
  float r = 0;
  for (auto vec : positions) {
    float rCan = std::abs(vec.x());


    rCan = std::abs(vec.x());
    if (rCan > r) {
      r = rCan;
    }

    rCan = std::abs(vec.x());
    if (rCan > r) {
      r = rCan;
    }
  }

  //compute unit sphere
  std::vector<QVector3D> nPos;
  for (auto vec : positions) {
    float n = sqrt(vec.x() * vec.x() + vec.y() * vec.y()
      + vec.z() * vec.z());
    nPos.push_back(vec / n);
  }

  //compute radiuses
  std::vector<float> rads;
  for (auto vec : nPos) {
    rads.push_back(sqrt(vec.x() * vec.x() + vec.y() * vec.y()
      + vec.z() * vec.z()));
  }

  //compute thetas
  int i = 0;
  std::vector<float> thetas;
  for (auto vec : nPos) {
    float theta = radToDeg(acos(vec.z() / rads[i]));
    theta = (vec.x() >= 0) ? theta : -1 * theta;
    thetas.push_back(theta);

    i++;
  }

  //compute phis
  std::vector<float> phis;
  for (auto vec : nPos) {
    //TODO: divide by zero?
    float phi = radToDeg(atan(vec.y() / vec.x()));
    phi = (vec.x() == 0) ? -1 * phi : phi;
    phis.push_back(phi);
  }

  // project 3D vector to 2D
  i = 0;
  for (auto vec : nPos) {
    float newX = degToRad(thetas[i]) * cos(degToRad(phis[i]));
    float newY = degToRad(thetas[i]) * sin(degToRad(phis[i]));

    positionsProjected.push_back(QVector2D(radToDeg(newX), radToDeg(newY)));
    //positionsProjected.push_back(QVector2D(positions[i].x(), positions[i].y()));
    i++;
  }

  // normalize positions for opengl
  //TODO: this should be in scalpcanvas
  float maxValue = 0;
  for (auto vec : positionsProjected) {
    if (std::abs(vec.x()) > maxValue)
      maxValue = std::abs(vec.x());

    if (std::abs(vec.y()) > maxValue)
      maxValue = std::abs(vec.y());
  }

  //TODO: temp scaling
  maxValue *= 1.5;

  for_each(positionsProjected.begin(), positionsProjected.end(), [maxValue](auto& v)
  {
    v *= 1 / maxValue;
  });
}
