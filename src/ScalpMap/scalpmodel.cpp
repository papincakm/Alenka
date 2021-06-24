#include "scalpmodel.h"

//TODO: delete later
#include <iostream>
#include <string>

// method fitSphere is a reimplementation of pseudocode written by David Eberly
// written in his work Least Squares Fitting of Data by Linear or QuadraticStructures, p. 21
// link -> https://www.geometrictools.com/Documentation/LeastSquaresFitting.pdf
// the work is licenced under Creative Commons Attribution 4.0 International License
// https://creativecommons.org/licenses/by/4.0/
bool ScalpModel::fitSpehere(std::vector<QVector3D> points, QVector3D& center, float& radius) {
  QVector3D average = { 0, 0, 0 };
  for (size_t i = 0; i < points.size(); i++) {
    average += points[i];
  }
  average /= points.size();

  float m00 = 0, m01 = 0, m02 = 0, m11 = 0, m12 = 0, m22 = 0;
  QVector3D r = { 0.0f, 0.0f, 0.0f };
  for (size_t i = 0; i < points.size(); i++) {
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
  }
  else {
    center = { 0, 0, 0 };
    radius = 0;
    return false;
  }
}

QVector3D ScalpModel::projectPoint(const QVector3D& point, const QVector3D& sphereCenter, const float radius) {
  QVector3D projectedPoint = point - sphereCenter;

  float len = projectedPoint.length();

  projectedPoint *= (radius / len);

  return projectedPoint + sphereCenter;
}
/**
* @brief Returned empty vector indicates that positions cant be projected.
*/
std::vector<QVector2D> ScalpModel::getPositionsProjected(const std::vector<QVector3D>& positions) {
  std::vector<QVector2D> resultPositions;
  QVector3D sphereCenter = { 0, 0, 0 };
  float radius = 0;

  if (!fitSpehere(positions, sphereCenter, radius)) {
    return resultPositions;
  }

  QVector3D fittedSphere = sphereCenter / -2.0f;

  float r = std::sqrt((sphereCenter.x() * sphereCenter.x() + sphereCenter.y() * sphereCenter.y() +
    sphereCenter.z() * sphereCenter.z()) / 4.0f - radius * 2);

  std::vector<QVector3D> positionsProjectedOnSphere;
  for (size_t i = 0; i < positions.size(); i++) {
    positionsProjectedOnSphere.push_back(projectPoint(positions[i], fittedSphere, r));
  }

  for (size_t i = 0; i < positions.size(); i++) {
    QVector2D newVec = { 0, 0 };

    if (useStereographicProjection) {
      newVec.setX(positionsProjectedOnSphere[i].x() / (r - positionsProjectedOnSphere[i].z()));
      newVec.setY(positionsProjectedOnSphere[i].y() / (r - positionsProjectedOnSphere[i].z()));
    }
    else {
      newVec.setX(positionsProjectedOnSphere[i].x());
      newVec.setY(positionsProjectedOnSphere[i].y());
    }

    resultPositions.push_back(newVec);
  }

  normalize(resultPositions);

  return resultPositions;
}

void ScalpModel::normalize(std::vector<QVector2D>& points) {
  float maxX = FLT_MIN;
  float maxY = FLT_MIN;
  float minX = FLT_MAX;
  float minY = FLT_MAX;

  for (auto vec : points) {
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

  for_each(points.begin(), points.end(), [scale, maxX, maxY, minX, minY](auto& v)
  {
    v.setX(v.x() - ((maxX + minX) / 2.0f));
    v.setY(v.y() - ((maxY + minY) / 2.0f));

    //rescale
    v.setX(v.x() / scale * 1.7f);
    v.setY(v.y() / scale * 1.7f);
  });
}