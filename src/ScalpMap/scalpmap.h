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
//struct CanvasTrack;

/**
* @brief Implements 2D scalp map.
*/
class ScalpMap : public QWidget {
  Q_OBJECT
  OpenDataFile *file = nullptr;
  ScalpCanvas *scalpCanvas = nullptr;
  ScalpModel model;
  float voltageMin = 0.0f;
  float voltageMax = 0.0f;
  bool parentVisible = true;
  //TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
  //which will be referenced in here and trackLabelBar
  std::vector<QMetaObject::Connection> trackTableConnections;
  std::vector<QMetaObject::Connection> fileInfoConnections;

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

private slots:
  void parentVisibilityChanged(bool vis);
  void updateTrackTableConnections(int row);
  void updateLabels();
  void updateSpectrum();
  void updateToExtremaCustom();
  void updateToExtremaLocal();
};

#endif // SCALPMAP_H