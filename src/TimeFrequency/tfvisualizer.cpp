#include "tfvisualizer.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../error.h"
#include "../openglprogram.h"
#include "../options.h"
#include "../myapplication.h"

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

#include <detailedexception.h>

//TODO: delete later
#include <memory>

#ifndef __APPLE__
#if defined WIN_BUILD
#include <windows.h>
#elif defined UNIX_BUILD
#include <GL/glx.h>
#endif
#endif

using namespace std;
using namespace AlenkaFile;
using namespace AlenkaSignal;

TfVisualizer::TfVisualizer(QWidget *parent) : QOpenGLWidget(parent) {
  printTiming = isProgramOptionSet("printTiming");

  //TODO: minor, initiate all rectangles here and update later in paint
  specMesh = graphics::SquareMesh(-0.8f, 0.7f, -0.7f, 0.8f);

  gradient = std::make_unique<graphics::Gradient>(
    graphics::Gradient(0.75f, 0.8f, specMesh.getYbot(), specMesh.getYtop(), this));
}

TfVisualizer::~TfVisualizer() {
  if (glInitialized)
    cleanup();
}

void TfVisualizer::deleteColormapTexture() {
  gl()->glBindTexture(GL_TEXTURE_1D, 0);

  gl()->glDeleteTextures(1, &colormapTextureId);
}

void TfVisualizer::cleanup() {
  makeCurrent();

  channelProgram.reset();

  OPENGL_INTERFACE->checkGLErrors();

  doneCurrent();
}

GLuint TfVisualizer::setupColormapTexture(std::vector<float> colormap) {
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

  return texId;
}

void TfVisualizer::updateColormapTexture() {
  gl()->glTexSubImage1D(GL_TEXTURE_1D, 0, 0, colormap.get().size() / 3, GL_RGB, GL_FLOAT, colormap.get().data());
  colormap.changed = false;
}


void TfVisualizer::initializeGL() {
  logToFile("Initializing OpenGL in TfVisualizer.");
  glInitialized = true;

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &TfVisualizer::cleanup);

  if (!OPENGL_INTERFACE)
  {
    OPENGL_INTERFACE = make_unique<OpenGLInterface>();
    OPENGL_INTERFACE->initializeOpenGLInterface();
  }

  QFile triangleVertFile(":/triangle.vert");
  triangleVertFile.open(QIODevice::ReadOnly);
  string triangleVert = triangleVertFile.readAll().toStdString();

  QFile triangleFragFile(":/triangle.frag");
  triangleFragFile.open(QIODevice::ReadOnly);
  string triangleFrag = triangleFragFile.readAll().toStdString();

  channelProgram = make_unique<OpenGLProgram>(triangleVert, triangleFrag);
}

void TfVisualizer::resizeGL(int /*w*/, int /*h*/) {
  gradient->update();
}

//converts array to range while keeping the same ratio between elements
void TfVisualizer::convertToRange(std::vector<float>& values, float newMin, float newMax) {
  float minVal = FLT_MAX;
  float maxVal = FLT_MIN;

  for (size_t i = 0; i < values.size(); i++) {
    minVal = std::min(minVal, values[i]);
    maxVal = std::max(maxVal, values[i]);
  }

  float newRange = newMax - newMin;
  float oldRange = maxVal - minVal;

  for (size_t i = 0; i < values.size(); i++) {
    values[i] = (values[i] - minVal) * newRange / oldRange + newMin;
  }
}

void TfVisualizer::setDataToDraw(std::vector<float> values, float xCount, float yCount) {
  decltype(std::chrono::high_resolution_clock::now()) start;
  if (printTiming) {
    start = std::chrono::high_resolution_clock::now();
  }

  //drawing points as squares in grid
  int xVertices = xCount + 1;
  int yVertices = yCount + 1;

  minGradVal = *std::min_element(values.begin(), values.end());
  maxGradVal = *std::max_element(values.begin(), values.end());

  //TODO: color of triangle with all three vertices at voltage=1 is bugged
  // triangle is drawn as purple for GL_LINEAR and arbitrarily red/blue for GL_NEAREST
  // thus the 0.9999f instead of 1
  convertToRange(values, 0.0f, 0.9999f);

  if (printTiming) {
    const std::chrono::nanoseconds time = std::chrono::high_resolution_clock::now() - start;
    currentBenchTimeGlobal += time;
  }

  if (specMesh.rows != yVertices || specMesh.columns != xVertices) {
    paintVertices.clear();
    paintIndices.clear();

    specMesh.rows = yVertices;
    specMesh.columns = xVertices;

    std::vector <float> xAxis = generateAxis(xVertices);
    convertToRange(xAxis, specMesh.getXleft(), specMesh.getXright());

    std::vector<float> yAxis = generateAxis(yVertices);
    convertToRange(yAxis, specMesh.getYbot(), specMesh.getYtop());

    generateTriangulatedGrid(paintVertices, paintIndices, xAxis, yAxis, values);

    //gradient
    gradient->generateGradientMesh(paintVertices, paintIndices);
  }
  else {
    if (printTiming) {
      start = std::chrono::high_resolution_clock::now();
    }

    //update values in posBuffer
    for (int i = 0; i < xCount; i++) {
      for (int j = 0; j < yCount; j++) {
        int valueIt = i * yCount + j;
        int valuePos = 2;
        for (int k = 0; k < 4; k++) {
          paintVertices[i * yCount * 12 + j * 12 + valuePos] = values[valueIt];
          valuePos += 3;
        }
      }
    }

    if (printTiming) {
      const std::chrono::nanoseconds time = std::chrono::high_resolution_clock::now() - start;
      currentBenchTimeGlobal += time;
    }
  }
}

void TfVisualizer::setSeconds(int secs) {
  seconds = secs;
}

void TfVisualizer::setFrequency(int fs) {
  frequency = fs;
}

void TfVisualizer::setMinFrequency(int fs) {
  minFreqDraw = fs;
}

void TfVisualizer::setMaxFrequency(int fs) {
  maxFreqDraw = fs;
}

int TfVisualizer::getMinFrequency() {
  return minFreqDraw;
}

int TfVisualizer::getMaxFrequency() {
  return maxFreqDraw;
}

void TfVisualizer::paintGL() {
  if (paintingDisabled)
    return;

  decltype(chrono::high_resolution_clock::now()) start;
  if (printTiming) {
    start = chrono::high_resolution_clock::now();
  }

  if (ready()) {
#ifndef NDEBUG
    logToFile("Painting started.");
#endif
    painter = new QPainter(this);

    auto specWindow = graphics::Rectangle(specMesh, this);
    specWindow.render(painter);

    gradient->render(painter);

    auto gradAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
      graphics::LineChain(gradient->getXright(), gradient->getXright() + 0.02f, specMesh.getYbot(), specMesh.getYtop() + 0.0024f, this, 5,
        graphics::Orientation::Vertical)
      );

    gradAxisLines->render(painter);

    auto gradAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
      graphics::NumberRange(gradient->getXright() + 0.05f, gradient->getXright() + 0.15f, specMesh.getYbot() + 0.04f,
        specMesh.getYtop() + 0.04f, this, 5, minGradVal, maxGradVal, 2, QColor(0, 0, 0),
        graphics::Orientation::Vertical)
      );
    gradAxisNumbers->render(painter);

    //frequency
    float specYSize = std::abs(specMesh.getYtop() - specMesh.getYbot());
    auto frequencyAxisLabel = graphics::RectangleText(graphics::Rectangle(-0.99f, -0.92f, specMesh.getYbot() + specYSize / 3.0f,
      specMesh.getYtop() - specYSize / 3.0f, this, graphics::Orientation::Vertical), "Arial", QColor(0, 0, 0), "Frequency (Hz)",
      graphics::Orientation::Vertical);
    frequencyAxisLabel.render(painter);

    auto frequencyAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
      graphics::NumberRange(specMesh.getXleft() - 0.10f, specMesh.getXleft() - 0.05f, specMesh.getYbot() + 0.04f,
        specMesh.getYtop() + 0.04f, this, 5, minFreqDraw, maxFreqDraw, 0, QColor(0, 0, 0), graphics::Orientation::Vertical)
      );
    frequencyAxisNumbers->render(painter);

    auto frameAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
      graphics::LineChain(specMesh.getXleft() - 0.02f, specMesh.getXleft(), specMesh.getYbot(),
        specMesh.getYtop() + 0.0024f, this, 5, graphics::Orientation::Vertical)
      );

    frameAxisLines->render(painter);

    //time
    auto timeAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
      graphics::LineChain(specMesh.getXleft(), specMesh.getXright() + 0.0015f, specMesh.getYbot() - 0.02f,
        specMesh.getYbot(), this, 5, graphics::Orientation::Horizontal, graphics::Orientation::Vertical)
      );

    timeAxisLines->render(painter);

    auto timeAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
      graphics::NumberRange(specMesh.getXleft() - 0.03f, specMesh.getXright() - 0.03f, specMesh.getYbot() - 0.15f,
        specMesh.getYbot() - 0.05f, this, 5, -seconds / 2, seconds / 2, 1, QColor(0, 0, 0), graphics::Orientation::Horizontal,
        graphics::Orientation::Horizontal)
      );
    timeAxisNumbers->render(painter);

    float specXSize = std::abs(specMesh.getXleft() - specMesh.getXright());
    auto timeAxisLabel = graphics::RectangleText(graphics::Rectangle(specMesh.getXleft() + specXSize / 2.4f,
      specMesh.getXright() - specXSize / 2.4f, specMesh.getYbot() - 0.25f, specMesh.getYbot() - 0.18f,
      this, graphics::Orientation::Vertical), "Arial", QColor(0, 0, 0), "Time(sec)", 
      graphics::Orientation::Horizontal);
    timeAxisLabel.render(painter);

    painter->beginNativePainting();

    GLfloat red = palette().color(QPalette::Window).redF();
    GLfloat green = palette().color(QPalette::Window).greenF();
    GLfloat blue = palette().color(QPalette::Window).blueF();

    gl()->glClearColor(red, green, blue, 1.0f);

    gl()->glUseProgram(channelProgram->getGLProgram());

    colormapTextureId = setupColormapTexture(colormap.get());

    genBuffers();

    gl()->glBufferData(GL_ARRAY_BUFFER, paintVertices.size() * sizeof(GLfloat), &paintVertices[0], GL_DYNAMIC_DRAW);

    // 1st attribute buffer : vertices
    //current position
    gl()->glEnableVertexAttribArray(0);
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void*)0);

    gl()->glEnableVertexAttribArray(1);
    gl()->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (char*)(sizeof(GLfloat) * 2));

    gl()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, paintIndices.size() * sizeof(GLuint), &paintIndices[0], GL_DYNAMIC_DRAW);

    gl()->glDrawElements(GL_TRIANGLES, paintIndices.size(), GL_UNSIGNED_INT, nullptr);

    for (int i = 0; i < 2; i++) {
      gl()->glDisableVertexAttribArray(i);
    }

    deleteBuffers();
    deleteColormapTexture();

    painter->endNativePainting();
    painter->end();

    if (printTiming) {
      const std::chrono::nanoseconds time = std::chrono::high_resolution_clock::now() - start;
      currentBenchTimeGlobal += time;
    }

#ifndef NDEBUG
    logToFile("Painting finished.");
#endif
  }
}

void TfVisualizer::genBuffers() {
  gl()->glGenBuffers(1, &indexBuffer);
  gl()->glGenBuffers(1, &posBuffer);

  gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
  gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

void TfVisualizer::deleteBuffers() {
  gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
  gl()->glDeleteBuffers(1, &posBuffer);

  gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl()->glDeleteBuffers(1, &indexBuffer);
}

bool TfVisualizer::ready() {
  return paintVertices.size();
}

std::vector<float> TfVisualizer::generateAxis(int pointCount) {
  std::vector<float> axis;
  //generating axis on grid so + 1 line at the end
  float pc = pointCount;
  for (int i = 0; i < pc; i++) {
    axis.push_back(i / pc);
  }
  return axis;
}

void TfVisualizer::generateTriangulatedGrid(std::vector<GLfloat>& triangles,
  std::vector<GLuint>& indices, const std::vector<float> xAxis, const std::vector<float> yAxis,
  const std::vector<float>& values) {

  for (size_t i = 0; i < xAxis.size() - 1; i++) {
    for (size_t j = 0; j < yAxis.size() - 1; j++) {
      int valueIt = i * (yAxis.size() - 1) + j;
      GLuint squarePos = i * (yAxis.size() - 1) * 4 + j * 4;
      //create rectangle from 2 triangles
      //first triangle, left bot vertex
      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j]);
      triangles.push_back(values[valueIt]);
      indices.push_back(squarePos);

      //right bot vertex
      triangles.push_back(xAxis[i + 1]);
      triangles.push_back(yAxis[j]);
      triangles.push_back(values[valueIt]);
      indices.push_back(squarePos + 1);

      //right top vertex
      triangles.push_back(xAxis[i + 1]);
      triangles.push_back(yAxis[j + 1]);
      triangles.push_back(values[valueIt]);
      indices.push_back(squarePos + 2);

      //second triangle, left bot vertex
      indices.push_back(squarePos);

      //left top vertex
      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j + 1]);
      triangles.push_back(values[valueIt]);
      indices.push_back(squarePos + 3);

      //right top vertex
      indices.push_back(squarePos + 2);
    }
  }
}

void TfVisualizer::renderPopupMenu(const QPoint& pos) {
  auto menu = colormap.getColormapMenu(this);

  menu->exec(mapToGlobal(pos));
}

void TfVisualizer::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && gradient->contains(event->pos())) {
    gradient->clicked(event->pos());
  }
}

void TfVisualizer::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (event->button() == Qt::MiddleButton && gradient->contains(event->pos()))
  {
    colormap.reset();
    gradient->reset();
    update();
  }
}

void TfVisualizer::mouseMoveEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->change(colormap, event->pos());
    update();
  }
}

void TfVisualizer::mouseReleaseEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->released();
  }
  else if (event->button() == Qt::RightButton)
  {
    renderPopupMenu(event->pos());
  }
}
