#ifndef SCALPMODEL_H
#define SCALPMODEL_H

#include <QVector2D>
#include <QVector3D>
#include <memory>

#include <vector>

/**
* @brief Implements 2D scalp map model.
*/
class ScalpModel {

public:
  bool useStereographicProjection = true;

  ScalpModel(){};

  std::vector<QVector2D> getPositionsProjected(const std::vector<QVector3D>& positions);

private:
  bool fitSpehere(std::vector<QVector3D> points, QVector3D& center, float& radius);
  QVector3D projectPoint(const QVector3D& point, const QVector3D& sphereCenter, const float radius);
  void scaleProjected(std::vector<QVector2D>& points);
};

#endif // SCALPMODEL_H
