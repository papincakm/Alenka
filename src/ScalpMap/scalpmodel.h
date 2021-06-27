#ifndef SCALPMODEL_H
#define SCALPMODEL_H

#include <QVector2D>
#include <QVector3D>
#include <memory>

#include <vector>

/**
* @brief Electrode projection calculation.
*/
class ScalpModel {

public:
  bool useStereographicProjection = true;

  ScalpModel(){};

  std::vector<QVector2D> getPositionsProjected(const std::vector<QVector3D>& positions);

private:
  /**
  * @brief Generates a sphere thats closes to a scattered set of points.
  */
  bool fitSpehere(std::vector<QVector3D> points, QVector3D& center, float& radius);
  /**
  * @brief Projects points onto a sphere.
  */
  QVector3D projectPoint(const QVector3D& point, const QVector3D& sphereCenter, const float radius);
  void normalize(std::vector<QVector2D>& points);
};

#endif // SCALPMODEL_H
