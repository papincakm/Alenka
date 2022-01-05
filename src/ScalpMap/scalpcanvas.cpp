#include "scalpcanvas.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../error.h"
#include "../myapplication.h"
#include "../openglprogram.h"
#include "../options.h"

#include <QCursor>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QOpenGLDebugLogger>
#include <QUndoCommand>
#include <QWheelEvent>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include <QVector2D>
#include <delaunator.hpp>

#include <detailedexception.h>

#ifndef __APPLE__
#if defined WIN_BUILD
#include <windows.h>
#elif defined UNIX_BUILD
#include <GL/glx.h>
#endif
#endif

#define OPENGL_VERTEX_SIZE 3
#define SPATIAL_P 1
#define SPATIAL_NEAREST_NEIGHBOUR_COUNT 3
#define TRIANGLE_SPLIT_COUNT 2

using namespace std;
using namespace AlenkaFile;
using namespace AlenkaSignal;

ScalpCanvas::ScalpCanvas(QWidget *parent) : QOpenGLWidget(parent) {
  printTiming = isProgramOptionSet("printTiming");

  gradient = std::make_unique<graphics::Gradient>(
    graphics::Gradient(0.85f, 0.88f, -0.9f, -0.4f, this, QColor(255, 255, 255), QColor(0, 0, 0)));
}

ScalpCanvas::~ScalpCanvas() {
  logToFile("Destructor in ScalpCanvas.");
  if (glInitialized)
    cleanupGL();
}

void ScalpCanvas::forbidDraw(const QString& errorString) {
  errorMsg = errorString;
  dataReadyToDraw = false;
}

void ScalpCanvas::allowDraw() {
  errorMsg = "";
  dataReadyToDraw = true;
}

void ScalpCanvas::clear() {
  originalPositions.clear();
  scalpMesh.clear();

  update();
}

void ScalpCanvas::initializeGL() {
  logToFile("Initializing OpenGL in ScalpCanvas.");
  glInitialized = true;
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScalpCanvas::cleanupGL);

  if (!OPENGL_INTERFACE) {
    OPENGL_INTERFACE = make_unique<OpenGLInterface>();
    OPENGL_INTERFACE->initializeOpenGLInterface();
  }

  //TODO: rename to circle or point
  QFile pointVertFile(":/single.vert");
  pointVertFile.open(QIODevice::ReadOnly);
  string pointVert = pointVertFile.readAll().toStdString();

  QFile circleFragFile(":/circle.frag");
  circleFragFile.open(QIODevice::ReadOnly);
  string circleFrag = circleFragFile.readAll().toStdString();

  labelProgram = make_unique<OpenGLProgram>(pointVert, circleFrag);

  QFile triangleVertFile(":/triangle.vert");
  triangleVertFile.open(QIODevice::ReadOnly);
  string triangleVert = triangleVertFile.readAll().toStdString();

  QFile triangleFragFile(":/triangle.frag");
  triangleFragFile.open(QIODevice::ReadOnly);
  string triangleFrag = triangleFragFile.readAll().toStdString();

  channelProgram = make_unique<OpenGLProgram>(triangleVert, triangleFrag);

  checkGLMessages();
}

void ScalpCanvas::cleanupGL() {
  logToFile("Cleanup in ScalpCanvas.");
  makeCurrent();

  channelProgram.reset();

  labelProgram.reset();

  logLastGLMessage();
  OPENGL_INTERFACE->checkGLErrors();

  doneCurrent();
}

void ScalpCanvas::setupScalpMesh() {
  auto triangulatedPositions = generateTriangulatedGrid(originalPositions);
  scalpMesh = generateScalpTriangleArray(triangulatedPositions);
  calculateSpatialCoefficients(scalpMesh);
  calculateVoltages(scalpMesh);

  gradient->generateGradientMesh(scalpMesh, indices);
  uniqueIndiceCount = scalpMesh.size();
}

void ScalpCanvas::setChannelPositions(const std::vector<QVector2D>& channelPositions) {
  originalPositions.clear();
  scalpMesh.clear();

  for (auto v : channelPositions) {
    //transform positions to better fit the window
    originalPositions.push_back(ElectrodePosition(-1 * v.y() - 0.05f, v.x()));
  }

  setupScalpMesh();
}

void ScalpCanvas::setChannelLabels(const std::vector<QString>& channelLabels) {
  labels = channelLabels;
}

void ScalpCanvas::setPositionVoltages(const std::vector<float>& channelDataBuffer, const float& min, const float& max) {
  //if (static_cast<int>(originalPositions.size()) < static_cast<int>(channelDataBuffer.size()))
  //return;

  minVoltage = min;
  maxVoltage = max;

  float maxMinusMin = maxVoltage - minVoltage;

  int size = std::min(originalPositions.size(), channelDataBuffer.size());

  for (int i = 0; i < size; i++) {
    float newVoltage = (channelDataBuffer[i] - minVoltage) / (maxMinusMin);

    //TODO: color of triangle with all three vertices at voltage=1 is bugged
    // triangle is drawn as purple for GL_LINEAR and arbitrarily red/blue for GL_NEAREST
    // thus the 0.9999f instead of 1
    originalPositions[i].voltage = newVoltage < 0 ? 0 : (newVoltage >= 1 ? 0.9999f : newVoltage);
  }

  calculateVoltages(scalpMesh);
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  gradient->update();
}

std::vector<ElectrodePosition> ScalpCanvas::generateTriangulatedGrid(
  const std::vector<ElectrodePosition>& channels) {
  std::vector<double> coords;
  std::vector<ElectrodePosition> triangles;

  for (auto ch : channels) {
    coords.push_back(ch.x);
    coords.push_back(ch.y);
  }

  try {
    delaunator::Delaunator d(coords);

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
      triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i]],
        d.coords[2 * d.triangles[i] + 1]));
      triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i + 1]],
        d.coords[2 * d.triangles[i + 1] + 1]));
      triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i + 2]],
        d.coords[2 * d.triangles[i + 2] + 1]));
    }
  }
  catch (...) {
    forbidDraw("Can't compute triangulation of the input coordinates.");
  }

  return triangles;
}

float getDistance(const float x1, const float y1, const float x2, const float y2) {
  return std::sqrt((x1 - x2) * (x1 - x2) +
    (y1 - y2) * (y1 - y2));
}

//takes triangulated positions, where each position is represented by 3 floats - x, y, z
//calculates spatial coefficients for each point in the mesh, based on its k nearest neighbours
void ScalpCanvas::calculateSpatialCoefficients(const std::vector<GLfloat>& triangulatedPoints) {
  pointSpatialCoefficients.clear();

  for (size_t i = 0; i < triangulatedPoints.size(); i += OPENGL_VERTEX_SIZE) {
    std::vector<std::pair<float, int>> distances;
    std::vector<PointSpatialCoefficient> singlePointSpatialCoefficients;

    for (size_t j = 0; j < originalPositions.size(); j++) {
      float distance = getDistance(triangulatedPoints[i], triangulatedPoints[i + 1], originalPositions[j].x, originalPositions[j].y);
      distance = (distance == 0) ? 0.0001f : distance;
      distances.push_back(std::make_pair(1.0f / (distance * distance), j));
    }

    std::sort(distances.begin(), distances.end());

    for (size_t j = distances.size() - 1; j > distances.size() - SPATIAL_NEAREST_NEIGHBOUR_COUNT - 1; j--) {
      float pDist = distances[j].first;
      for (int s = 1; s < SPATIAL_P; s++) {
        pDist = pDist * distances[j].first;
      }

      singlePointSpatialCoefficients.push_back(PointSpatialCoefficient(pDist, distances[j].second));
    }

    pointSpatialCoefficients.push_back(singlePointSpatialCoefficients);
  }
}

void ScalpCanvas::calculateVoltages(std::vector<GLfloat>& points) {
  //points contain gradient mesh as well
  assert(static_cast<int>(points.size() / OPENGL_VERTEX_SIZE) >= static_cast<int>(pointSpatialCoefficients.size()));

  for (size_t i = 0; i < pointSpatialCoefficients.size(); i++) {
    float newVoltage = 0;
    float sumCoefficient = 0;
    for (size_t j = 0; j < pointSpatialCoefficients[i].size(); j++) {
      newVoltage += pointSpatialCoefficients[i][j].coefficient *
        originalPositions[pointSpatialCoefficients[i][j].toPoint].voltage;
      sumCoefficient += pointSpatialCoefficients[i][j].coefficient;
    }
    newVoltage /= sumCoefficient;

    points[i * OPENGL_VERTEX_SIZE + 2] = newVoltage;
  }
}

std::vector<GLfloat> ScalpCanvas::generateScalpTriangleArray(
  const std::vector<ElectrodePosition>& triangulatedPositions) {
  indices.clear();
  uniqueIndiceCount = 0;
  std::vector<ElectrodePosition> positions;

  for (size_t i = 0; i < triangulatedPositions.size(); i++) {
    //indices
    bool duplicate = false;
    for (int j = 0; j < i; j++) {
      if (triangulatedPositions[i] == triangulatedPositions[j]) {
        duplicate = true;
        indices.push_back(indices[j]);
        break;
      }
    }
    if (!duplicate) {
      indices.push_back(uniqueIndiceCount++);
      positions.push_back(triangulatedPositions[i]);
    }
  }

  std::vector<ElectrodePosition> splitPositions = std::move(positions);
  for (int i = 0; i < TRIANGLE_SPLIT_COUNT; i++) {
    splitTriangles(splitPositions, indices);
  }

  //transform ElectrodePositions into GLFloats
  std::vector<GLfloat> finalTriangles;
  for (auto s : splitPositions) {
    finalTriangles.push_back(s.x);
    finalTriangles.push_back(s.y);
    finalTriangles.push_back(s.voltage);
  }

  return finalTriangles;
}

GLuint ScalpCanvas::setupColormapTexture(std::vector<float> colormap) {
  GLuint texId;
  gl()->glGenTextures(1, &texId);
  gl()->glBindTexture(GL_TEXTURE_1D, texId);

  gl()->glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, colormap.size() / 3, 0, GL_RGB, GL_FLOAT, colormap.data());

  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLuint samplerLocation = gl()->glGetUniformLocation(channelProgram->getGLProgram(), "colormap");
  gl()->glUniform1i(samplerLocation, 0);

  gl()->glActiveTexture(GL_TEXTURE0);
  gl()->glBindTexture(GL_TEXTURE_1D, texId);

  gl()->glFinish();

  return texId;
}

void ScalpCanvas::updateColormapTexture() {
  gl()->glTexSubImage1D(GL_TEXTURE_1D, 0, 0, colormap.get().size() / 3, GL_RGB, GL_FLOAT, colormap.get().data());
  colormap.changed = false;
}

//TODO: doesnt refresh on table change
void ScalpCanvas::paintGL() {
#ifndef NDEBUG
  logToFile("Painting started.");
#endif
  if (ready()) {
    decltype(chrono::high_resolution_clock::now()) start;
    if (printTiming) {
      start = chrono::high_resolution_clock::now();
    }

    painter = new QPainter(this);
    painter->beginNativePainting();

    if (scalpMesh.empty()) {
      setupScalpMesh();
    }

    gl()->glEnable(GL_PROGRAM_POINT_SIZE);
    gl()->glEnable(GL_POINT_SPRITE);

    gl()->glUseProgram(channelProgram->getGLProgram());

    colormapTextureId = setupColormapTexture(colormap.get());

    genBuffers();

    gl()->glBufferData(GL_ARRAY_BUFFER, scalpMesh.size() * sizeof(GLfloat), &scalpMesh[0], GL_DYNAMIC_DRAW);

    // 1st attribute buffer : vertices
    //current position
    gl()->glEnableVertexAttribArray(0);
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * OPENGL_VERTEX_SIZE, (void*)0);

    gl()->glEnableVertexAttribArray(1);
    gl()->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * OPENGL_VERTEX_SIZE, (char*)(sizeof(GLfloat) * 2));

    gl()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_DYNAMIC_DRAW);
    gl()->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    for (int i = 0; i < 2; i++) {
      gl()->glDisableVertexAttribArray(i);
    }
    checkGLMessages();
    deleteBuffers();

    //POINTS
    if (shouldDrawChannels) {
      gl()->glPointSize(10.0f);
    std::vector<GLfloat> channelBufferData;
      for (size_t i = 0; i < originalPositions.size(); i++) {
        channelBufferData.push_back(originalPositions[i].x);
        channelBufferData.push_back(originalPositions[i].y);
      }
      gl()->glUseProgram(labelProgram->getGLProgram());
      gl()->glGenBuffers(1, &pointBuffer);
      gl()->glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
      gl()->glBufferData(GL_ARRAY_BUFFER, channelBufferData.size() * sizeof(GLfloat), &channelBufferData[0], GL_DYNAMIC_DRAW);

      gl()->glEnableVertexAttribArray(0);
      gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

      gl()->glDrawArrays(GL_POINTS, 0, channelBufferData.size() / 2);
      gl()->glDisableVertexAttribArray(0);

      gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
      gl()->glDeleteBuffers(1, &pointBuffer);
    }
    gl()->glBindTexture(GL_TEXTURE_1D, 0);
    gl()->glDeleteTextures(1, &colormapTextureId);

    gl()->glDisable(GL_PROGRAM_POINT_SIZE);
    gl()->glDisable(GL_POINT_SPRITE);

    checkGLMessages();

    painter->endNativePainting();

    if (shouldDrawLabels) {
      for (size_t i = 0; i < originalPositions.size(); i++) {
        QFont labelFont = QFont("Times", 8, QFont::Bold);

        renderText(painter, originalPositions[i].x, originalPositions[i].y, labels[i], labelFont, QColor(255, 255, 255));
      }
    }

    //QPainter part
    auto gradAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
      graphics::NumberRange(gradient->getXright() + 0.01f, gradient->getXright() + 0.10f, gradient->getYbot() + 0.04f,
        gradient->getYtop() + 0.04f, this, 3, minVoltage, maxVoltage, 2, QColor(255, 255, 255), QColor(0, 0, 0),
        graphics::Orientation::Vertical)
      );
    gradAxisNumbers->render(painter);

    gradient->render(painter);

    if (printTiming) {
      const std::chrono::nanoseconds time = std::chrono::high_resolution_clock::now() - start;
      currentBenchTimeGlobal += time;
    }

    painter->end();
  }
  else {
    painter = new QPainter(this);
    renderErrorMsg();

    painter->end();
  }

#ifndef NDEBUG
  logToFile("Painting finished.");
#endif
}

void ScalpCanvas::genBuffers() {
  gl()->glGenBuffers(1, &indexBuffer);
  gl()->glGenBuffers(1, &posBuffer);

  gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
  gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

void ScalpCanvas::deleteBuffers() {
  gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
  gl()->glDeleteBuffers(1, &posBuffer);

  gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl()->glDeleteBuffers(1, &indexBuffer);
}

void ScalpCanvas::renderErrorMsg() {
  auto errorFont = QFont("Times", 15, QFont::Bold);
  painter->save();
  painter->setBackground(QBrush(QColor(0, 0, 0)));
  painter->setPen(QColor(255, 0, 0));
  painter->setBrush(QColor(255, 0, 0));
  painter->setFont(errorFont);
  painter->drawText(this->rect(), Qt::AlignCenter, errorMsg);
  painter->restore();
}

void ScalpCanvas::logLastGLMessage() {
  if (lastGLMessageCount > 1) {
    logToFile("OpenGL message (" << lastGLMessageCount - 1 << "x): " << lastGLMessage);
  }
}

bool ScalpCanvas::ready() {
  return dataReadyToDraw && scalpMesh.size() > 0;
}

void ScalpCanvas::renderText(QPainter* painter, float x, float y, const QString& str, const QFont& font, const QColor& fontColor) {
  int realX = width() / 2 + (width() / 2) * x;
  int realY = height() / 2 + (height() / 2) * y * -1;

  painter->save();
  painter->setBackgroundMode(Qt::OpaqueMode);
  painter->setBackground(QBrush(QColor(0, 0, 0)));
  painter->setPen(fontColor);
  painter->setBrush(fontColor);
  painter->setFont(font);
  painter->drawText(realX, realY, str);
  painter->restore();
}

void ScalpCanvas::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && gradient->contains(event->pos())) {
    gradient->clicked(event->pos());
  }
}

void ScalpCanvas::mouseMoveEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->change(colormap, event->pos());
    update();
  }
}

void ScalpCanvas::mouseReleaseEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->released();
  }
  else if (event->button() == Qt::RightButton)
  {
    renderPopupMenu(event->pos());
  }
}

void ScalpCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (event->button() == Qt::MiddleButton && gradient->contains(event->pos()))
  {
    colormap.reset();
    gradient->reset();
    update();
  }
}

void ScalpCanvas::checkGLMessages() {
  auto logPtr = OPENGL_INTERFACE->log();

  if (logPtr) {
    for (const auto &m : logPtr->loggedMessages()) {
      string message = m.message().toStdString();

      if (message != lastGLMessage) {
        logLastGLMessage();
        lastGLMessageCount = 0;

        logToFile("OpenGL message: " << message);
      }

      lastGLMessage = message;
      ++lastGLMessageCount;
    }
  }
}

void ScalpCanvas::renderPopupMenu(const QPoint& pos) {
  QMenu* menu = new QMenu(this);

  //draw channels option
  QAction setChannelDrawing("Draw Channels", this);

  connect(&setChannelDrawing, &QAction::triggered, [this]() {
    shouldDrawChannels = !shouldDrawChannels;
    update();
  });

  menu->addAction(&setChannelDrawing);
  menu->actions().back()->setCheckable(true);
  menu->actions().back()->setChecked(shouldDrawChannels);

  //draw labels option
  QAction setLabelDrawing("Draw Labels", this);

  connect(&setLabelDrawing, &QAction::triggered, [this]() {
    shouldDrawLabels = !shouldDrawLabels;
    update();
  });

  menu->addAction(&setLabelDrawing);
  menu->actions().back()->setCheckable(true);
  menu->actions().back()->setChecked(shouldDrawLabels);

  menu->addSeparator();

  menu->addMenu(colormap.getColormapMenu(this));
  menu->addMenu(setupProjectionMenu());
  menu->addSeparator();

  QMenu* extremaMenu = menu->addMenu("Extrema");
  auto* extremaGroup = new QActionGroup(this);
  extremaGroup->setExclusive(true);

  //set local extrema
  QAction setExtremaLocal("Local", this);

  connect(&setExtremaLocal, &QAction::triggered, [this]() {
    OpenDataFile::infoTable.setExtremaLocal();
  });
  setExtremaLocal.setCheckable(true);

  extremaMenu->addAction(&setExtremaLocal);
  extremaGroup->addAction(&setExtremaLocal);

  //set custom extrema
  QAction setExtremaCustom("Custom", this);

  connect(&setExtremaCustom, &QAction::triggered, [this]() {
    OpenDataFile::infoTable.setExtremaCustom();
  });
  setExtremaCustom.setCheckable(true);

  extremaMenu->addAction(&setExtremaCustom);
  extremaGroup->addAction(&setExtremaCustom);

  switch (OpenDataFile::infoTable.getScalpMapExtrema()) {
  case InfoTable::Extrema::custom:
    setExtremaCustom.setChecked(true);
    break;
  case InfoTable::Extrema::local:
    setExtremaLocal.setChecked(true);
    break;
  }

  menu->addMenu(extremaMenu);
  menu->addSeparator();

  menu->exec(mapToGlobal(pos));
}

QMenu* ScalpCanvas::setupProjectionMenu() {
  QMenu* projectionMenu = new QMenu("Electrode Projection");
  auto* projectionGroup = new QActionGroup(this);
  projectionGroup->setExclusive(true);

  //set no projection
  QAction* setProjectionSphere = new QAction("Sphere", this);

  connect(setProjectionSphere, &QAction::triggered, [this]() {
    OpenDataFile::infoTable.setScalpMapProjection(false);
  });
  setProjectionSphere->setCheckable(true);

  projectionMenu->addAction(setProjectionSphere);
  projectionGroup->addAction(setProjectionSphere);

  //set stereographic projection
  QAction* setProjectionStereo = new QAction("Stereographic", this);

  connect(setProjectionStereo, &QAction::triggered, [this]() {
    OpenDataFile::infoTable.setScalpMapProjection(true);
  });
  setProjectionStereo->setCheckable(true);

  projectionMenu->addAction(setProjectionStereo);
  projectionGroup->addAction(setProjectionStereo);

  switch (OpenDataFile::infoTable.getScalpMapProjection()) {
  case true:
    setProjectionStereo->setChecked(true);
    break;
  case false:
    setProjectionSphere->setChecked(true);
    break;
  }

  return projectionMenu;
}

GLuint ScalpCanvas::addMidEdgePoint(std::vector<ElectrodePosition>& splitTriangles,
  std::vector<GLuint>& splitIndices, ElectrodePosition candidate) {
  GLuint indice = 0;
  bool duplicate = false;
  for (size_t j = 0; j < splitTriangles.size(); j++) {
    if (candidate == splitTriangles[j]) {
      duplicate = true;
      indice = splitTriangles[j].indice;
      splitIndices.push_back(indice);
      break;
    }
  }
  if (!duplicate) {
    indice = uniqueIndiceCount;
    candidate.indice = indice;
    splitIndices.push_back(uniqueIndiceCount++);
    splitTriangles.push_back(candidate);
  }

  return indice;
}

// splits each triangle into 4 smaller triangles with the middle points of their edges as the new vertices,
// adds the new vertices at the end of the vertice array
// adds all indices and the end of indice array
void ScalpCanvas::splitTriangles(std::vector<ElectrodePosition>& triangleVertices,
  std::vector<GLuint>& indices) {
  std::vector<ElectrodePosition> splitTriangles;
  std::vector<GLuint> splitIndices;

  int oldIndicesSize = indices.size();

  for (int i = 0; i < oldIndicesSize; i += 3) {
    const int second = i + 1;
    const int third = i + 2;
    //1. triangle
    //1. vertex, 2. vertex
    float midPointAx = 0.5f * triangleVertices[indices[i]].x + 0.5f * triangleVertices[indices[second]].x;
    float midPointAy = 0.5f * triangleVertices[indices[i]].y + 0.5f * triangleVertices[indices[second]].y;
    float midPointAVol = 0.5f * triangleVertices[indices[i]].voltage + 0.5f * triangleVertices[indices[second]].voltage;
    //indices
    auto indiceA = addMidEdgePoint(splitTriangles, splitIndices,
      ElectrodePosition(midPointAx, midPointAy, midPointAVol));

    //1. vertex, 3. vertex
    float midPointBx = 0.5f * triangleVertices[indices[i]].x + 0.5f * triangleVertices[indices[third]].x;
    float midPointBy = 0.5f * triangleVertices[indices[i]].y + 0.5f * triangleVertices[indices[third]].y;
    float midPointBVol = 0.5f * triangleVertices[indices[i]].voltage + 0.5f * triangleVertices[indices[third]].voltage;
    auto indiceB = addMidEdgePoint(splitTriangles, splitIndices,
      ElectrodePosition(midPointBx, midPointBy, midPointBVol));

    //2. vertex, 3. vertex
    float midPointCx = 0.5f * triangleVertices[indices[second]].x + 0.5f * triangleVertices[indices[third]].x;
    float midPointCy = 0.5f * triangleVertices[indices[second]].y + 0.5f * triangleVertices[indices[third]].y;
    float midPointCVol = 0.5f * triangleVertices[indices[second]].voltage + 0.5f * triangleVertices[indices[third]].voltage;
    auto indiceC = addMidEdgePoint(splitTriangles, splitIndices,
      ElectrodePosition(midPointCx, midPointCy, midPointCVol));

    //2. triangle
    //1. vertex
    splitIndices.push_back(indices[i]);

    //A
    splitIndices.push_back(indiceA);

    //B
    splitIndices.push_back(indiceB);

    //3. triangle
    //2. vertex
    splitIndices.push_back(indices[second]);

    //A
    splitIndices.push_back(indiceA);

    //C
    splitIndices.push_back(indiceC);

    //4. triangle
    splitIndices.push_back(indices[third]);

    //B
    splitIndices.push_back(indiceB);

    //C
    splitIndices.push_back(indiceC);

  }

  triangleVertices.insert(triangleVertices.end(), splitTriangles.begin(), splitTriangles.end());
  indices.insert(indices.end(), splitIndices.begin(), splitIndices.end());
}
