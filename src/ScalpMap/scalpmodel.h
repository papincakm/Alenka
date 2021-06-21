#ifndef SCALPMODEL_H
#define SCALPMODEL_H

#include "../DataModel/infotable.h"

#include <QWidget>
#include <QColor>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <memory>


#include <vector>

class OpenDataFile;
class ScalpCanvas;
class InfoTable;

/**
* @brief Implements 2D scalp map model.
*/
class ScalpModel {
  OpenDataFile *file = nullptr;
  std::vector<QVector3D> positions;
  std::vector<QVector2D> positionsProjected;

public:
  ScalpModel(){};

private:
};

#endif // SCALPMODEL_H
