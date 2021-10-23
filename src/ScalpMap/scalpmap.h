#ifndef SCALPMAP_H
#define SCALPMAP_H

#include "../DataModel/infotable.h"
#include "scalpmodel.h"

#include <QWidget>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <memory>
#include <vector>

class OpenDataFile;
class ScalpCanvas;
class InfoTable;

/**
* @brief Recieves signals from global Qt contexts.
* Controls  the ScalpModel and Scalpcanvas to draw a scalp map.
*/
class ScalpMap : public QWidget {
  Q_OBJECT
  OpenDataFile *file = nullptr;
  ScalpCanvas *scalpCanvas = nullptr;
  ScalpModel model;
  float voltageMin = 0.0f;
  float voltageMax = 0.0f;
  bool parentVisible = true;
  bool printTiming = false;
  int posValidCnt = 0;

  std::vector<QMetaObject::Connection> trackTableConnections;
  std::vector<QMetaObject::Connection> fileInfoConnections;
  std::vector<bool> allowedPositions;

public:
  explicit ScalpMap(QWidget *parent = nullptr);

  /**
  * @brief Notifies this object that the DataFile changed.
  * @param file Pointer to the data file. nullptr means file was closed.
  */
  void changeFile(OpenDataFile *file);

private:
  void deleteTrackTableConnections();
  void deleteFileInfoConnections();
  void setupCanvas();
  void updateFileInfoConnections();
  bool positionsValid(const std::vector<QVector3D>& positions);
  bool enabled();
  void reset();

private slots:
  void parentVisibilityChanged(bool vis);
  void updateTrackTableConnections();
  void updateLabels();
  void updateSpectrum();
  void updateToExtremaCustom();
  void updateToExtremaLocal();
  void setScalpMapProjection(bool proj);
};

#endif // SCALPMAP_H
