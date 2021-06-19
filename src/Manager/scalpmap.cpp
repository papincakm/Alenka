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
  if (file) {
    updateFileInfoConnections();
    updateLabels();
  }


  //scalpCanvas->clear();

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
  if (!file)
    return;

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
  
  std::cout << "updating labels\n";
  
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

// this is reimplementation of https://www.geometrictools.com/Documentation/LeastSquaresFitting.pdf
// p. 21
bool fitSpehere(std::vector<QVector3D> points, QVector3D& center, float& radius) {
  QVector3D average = { 0, 0, 0 };
  for (size_t i = 0; i < points.size(); i++) {
    average += points[i];
  }
  average /= points.size();

  float m00 = 0, m01 = 0, m02 = 0, m11 = 0, m12 = 0, m22 = 0;
  QVector3D r = { 0.0f, 0.0f, 0.0f };
  for (size_t i = 0; i < points.size(); i++) {
    //QVector2D y(points[i].x() - average.x(), points[i].y() - average.y());
    QVector3D y = points[i] - average;
    float y0y0 = y.x() * y.x(), y0y1 = y.x() * y.y(), y0y2 = y.x() * y.z();
    float y1y1 = y.y() * y.y(), y1y2 = y.y() * y.z(), y2y2 = y.z() * y.z();
    m00 += y0y0;
    m01 += y0y1;
    m02 += y0y2;
    
    m11 += y1y1;
    m12 += y1y2;
    m22 += y2y2;

    r += (y0y0 + y1y1 + y2y2) * y;
  }
  r /= 2.0f;

  float cof00 = m11 * m22 - m12 * m12;
  float cof01 = m02 * m12 - m01 * m22;
  float cof02 = m01 * m12 - m02 * m11;
  float det = m00 * cof00 + m01 * cof01 + m02 * cof02;

  if (det != 0) {
    float cof11 = m00 * m22 - m02 * m02;
    float cof12 = m01 * m02 - m00 * m12;
    float cof22 = m00 * m11 - m01 * m01;

    center.setX(average.x() + (cof00 * r.x() + cof01 * r.y() + cof02 * r.z()) / det);
    center.setY(average.y() + (cof01 * r.x() + cof11 * r.y() + cof12 * r.z()) / det);
    center.setZ(average.z() + (cof02 * r.x() + cof12 * r.y() + cof22 * r.z()) / det);
    float rsqr = 0;

    for (size_t i = 0; i < points.size(); i++) {
      QVector3D delta = points[i] - center;
      rsqr += QVector3D::dotProduct(delta, delta);
    }
    rsqr /= points.size();
    radius = std::sqrt(rsqr);
    return true;
  } else {
    center = { 0, 0, 0 };
    radius = 0;
    return false;
  }
}

QVector3D projectPoint(const QVector3D& point, const QVector3D& sphereCenter, const float radius) {
  QVector3D projectedPoint = point - sphereCenter;

  float len = projectedPoint.length();

  projectedPoint *= (radius / len);

  return projectedPoint + sphereCenter;
}

//TODO: improvised algorithm, find a better way to do this
void ScalpMap::updatePositionsProjected() {
  positionsProjected.clear();
  QVector3D sphereCenter = {0, 0, 0};
  float radius = 0;
  if (!fitSpehere(positions, sphereCenter, radius)) {
    std::cout << "couldnt fite sphere\n";
  }

  QVector3D projectSphere(sphereCenter.x() / -2.0f, sphereCenter.y() / -2.0f, sphereCenter.z() / -2.0f);

  float r = std::sqrt((sphereCenter.x() * sphereCenter.x() + sphereCenter.y() * sphereCenter.y() +
    sphereCenter.z() * sphereCenter.z()) / 4.0f - radius * 2);

  std::vector<QVector3D> positionsProjectedOnSphere;
  for (size_t i = 0; i < positions.size(); i++) {
    positionsProjectedOnSphere.push_back(projectPoint(positions[i], projectSphere, r));
  }

  for (size_t i = 0; i < positions.size(); i++) {
    QVector2D newVec = { 0, 0 };
 
    newVec.setX(positionsProjectedOnSphere[i].x() / (r - positionsProjectedOnSphere[i].z()));
    newVec.setY(positionsProjectedOnSphere[i].y() / (r - positionsProjectedOnSphere[i].z()));
    positionsProjected.push_back(newVec);
  }

  // normalize positions for opengl
  //TODO: this should be in scalpcanvas
  float maxX = FLT_MIN;
  float maxY = FLT_MIN;
  float minX = FLT_MAX;
  float minY = FLT_MAX;

  for (auto vec : positionsProjected) {
    if (vec.x() > maxX)
      maxX = vec.x();

    if (vec.y() > maxY)
      maxY = vec.y();

    if (vec.x() < minX)
      minX = vec.x();

    if (vec.y() < minY)
      minY = vec.y();
  }

  float scale = std::max(maxX - minX, maxY - minY);
  std::cout << "scale = " << scale << "\n";

  for_each(positionsProjected.begin(), positionsProjected.end(), [scale, maxX, maxY, minX, minY](auto& v)
  {
    v.setX(v.x() - ((maxX + minX) / 2.0f));
    v.setY(v.y() - ((maxY + minY) / 2.0f));

    v.setX(v.x() / scale * 1.6f);
    v.setY(v.y() / scale * 1.6f);
  });
}
