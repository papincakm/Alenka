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
  connect(parent, SIGNAL(visibilityChanged(bool)),
    SLOT(parentVisibilityChanged(bool)));

  setupCanvas();
}

void ScalpMap::changeFile(OpenDataFile *file) {
  this->file = file;
  updateFileInfoConnections();
  //scalpCanvas->clear();
  updateLabels();
  /*if (enabled()) {
    if (!scalpCanvas) {
      //std::cout << "change file SETUPING canvas\n";
      setupCanvas();
    }
    else {
    }
  }*/
}

bool ScalpMap::enabled() {
  return (file && isVisible() && parentVisible);
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

  c = connect(&OpenDataFile::infoTable, SIGNAL(selectedMontageChanged(int)), this,
    SLOT(updateTrackTableConnections(int)));
}

void ScalpMap::parentVisibilityChanged(bool vis) {
  parentVisible = vis;
  if (vis)
    updateSpectrum();
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateTrackTableConnections(int row) {
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

void ScalpMap::setupCanvas() {
  auto box = new QVBoxLayout;
  setLayout(box);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);

  scalpCanvas = new ScalpCanvas(this);
  box->addWidget(scalpCanvas);
  setMinimumHeight(100);
  setMinimumWidth(100);
}

//TODO: copied from canvas.cpp
const AbstractTrackTable* getTrackTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->trackTable(
    OpenDataFile::infoTable.getSelectedMontage());
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateLabels() {
  if (!file || file->dataModel->montageTable()->rowCount() <= 0)
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
    scalpCanvas->forbidDraw("Electrode positions are invalid(Two positions can't have the same coordinates)."\
      "Change the coordinates or disable the invalid electrodes.");
    return;
  }

  updatePositionsProjected();

  scalpCanvas->setChannelLabels(labels);
  scalpCanvas->setChannelPositions(positionsProjected);

  if (!enabled())
    return;

  scalpCanvas->allowDraw();
}

//TODO: investigate where freq are stored if canvas cant draw
void ScalpMap::updateSpectrum() {
  if (!enabled())
    return;

  const AbstractTrackTable *trackTable =
    file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());

  if (trackTable->rowCount() <= 0)
    return;

  const int position = OpenDataFile::infoTable.getPosition();

  std::vector<float> samples = OpenDataFile::infoTable.getSignalSampleCurPosProcessed();

  if (OpenDataFile::infoTable.getScalpMapExtrema() == InfoTable::Extrema::local) {
    voltageMin = *std::min_element(std::begin(samples), std::end(samples));
    voltageMax = *std::max_element(std::begin(samples), std::end(samples));
  }

  scalpCanvas->setPositionVoltages(samples, voltageMin, voltageMax);
  scalpCanvas->allowDraw();
  scalpCanvas->update();
}

void ScalpMap::updateToExtremaLocal() {
  updateSpectrum();
}

void ScalpMap::updateToExtremaCustom() {
  QDialog* dialog = new QDialog(this);
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
    //TODO: check if min is lesser than max
    std::cout << "custom extrema accepted\n";
  });

  connect(buttonBox, &QDialogButtonBox::rejected, [this, dialog]() {
    std::cout << "custom extrema rejected\n";
    OpenDataFile::infoTable.setExtremaLocal();
    dialog->close();
  });

  dialog->exec();
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
