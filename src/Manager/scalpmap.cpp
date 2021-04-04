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
    SLOT(updateConnections(int)));

}

void ScalpMap::changeFile(OpenDataFile *file) {
  this->file = file;

  if (enabled()) {
    scalpCanvas->clear();
    updateLabels();
    //updateSpectrum();
  }
}

bool ScalpMap::enabled() {
  return (file && isVisible());
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateConnections(int row) {
  for (auto e : connections)
    disconnect(e);
  connections.clear();

  if (enabled()) {
    const AbstractMontageTable *mt = file->dataModel->montageTable();

    if (0 <= row && row < mt->rowCount()) {
      auto vitness = VitnessTrackTable::vitness(mt->trackTable(row));

      auto c = connect(vitness, SIGNAL(valueChanged(int, int)), this, SLOT(updateLabels()));
      connections.push_back(c);

      c = connect(vitness, SIGNAL(rowsInserted(int, int)), this, SLOT(updateLabels()));

      connections.push_back(c);
      c = connect(vitness, SIGNAL(rowsRemoved(int, int)), this,
        SLOT(updateLabels()));
      connections.push_back(c);

      /*c = connect(&file->infoTable, SIGNAL(positionChanged(int, double)), this,
      SLOT(updateSpectrum()));
      connections.push_back(c);*/

      c = connect(&file->infoTable, SIGNAL(signalCurPosProcessedChanged()),
        this, SLOT(updateSpectrum()));
      connections.push_back(c);

      updateLabels();
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
  if (scalpCanvas) {
    delete scalpCanvas;
  }

  QLayout* layout = this->layout();
  if (layout) {
    delete layout;
  }
}


void ScalpMap::showEvent(QShowEvent * event) {
  auto box = new QVBoxLayout;
  setLayout(box);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);

  scalpCanvas = new ScalpCanvas(this);
  box->addWidget(scalpCanvas);
  setMinimumHeight(100);
  setMinimumWidth(100);

  //TODO: Rethink this, there are redundant steps
  updateLabels();
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateLabels() {
  labels.clear();
  colors.clear();
  positions.clear();

  if (!file || file->dataModel->montageTable()->rowCount() <= 0)
    return;

  const AbstractTrackTable *trackTable =
    file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());

  if (trackTable->rowCount() <= 0)
    return;

  int track = 0;
  //TODO: what to do whith hidden channels
  //std::cout << "rowCount: " << trackTable->rowCount() << "  channelCount: " << file->file->getChannelCount() << "freqs:\n";
  //assert(static_cast<int>(trackTable->rowCount()) <= static_cast<int>(file->file->getChannelCount()));

  for (int i = 0; i < trackTable->rowCount(); ++i) {
    Track t = trackTable->row(i);

    if (!t.hidden) {
      //TODO: labels, colors and positions should be in one class and one vector
      labels.push_back(QString::fromStdString(t.label));
      colors.push_back(DataModel::array2color<QColor>(t.color));
      positions.push_back(QVector3D(t.x, t.y, t.z));

      ++track;
    }
  }

  if (!positionsValid() || trackTable->rowCount() < 3) {
    std::cout << "update labels returned" << std::endl;
    scalpCanvas->forbidDraw("Channel positions are invalid(Two positions can't be the same).");
    return;
  }

  updatePositionsProjected();

  scalpCanvas->setChannelLabels(labels);
  scalpCanvas->setChannelPositions(positionsProjected);

  scalpCanvas->allowDraw();

  //updateSpectrum();

  update();
  //scalpCanvas->update();
}

//copied from canvas.cpp
const AbstractTrackTable *getTrackTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->trackTable(
    OpenDataFile::infoTable.getSelectedMontage());
}

//TODO: investigate where freq are stored if canvas cant draw
void ScalpMap::updateSpectrum() {
  if (!file || !scalpCanvas)
    return;

  const AbstractTrackTable *trackTable =
    file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());

  if (trackTable->rowCount() <= 0)
    return;

  const int position = OpenDataFile::infoTable.getPosition();

  std::vector<float> signalSamplePosition = OpenDataFile::infoTable.getSignalSampleCurPosProcessed();

  auto min = std::min_element(std::begin(signalSamplePosition), std::end(signalSamplePosition));
  auto max = std::max_element(std::begin(signalSamplePosition), std::end(signalSamplePosition));

  scalpCanvas->setPositionFrequencies(signalSamplePosition, *min, *max);

  update();
  scalpCanvas->update();
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
