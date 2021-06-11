#include "tfvisualizer.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "DataModel/opendatafile.h"
#include "DataModel/undocommandfactory.h"
#include "DataModel/vitnessdatamodel.h"
#include "error.h"
#include "myapplication.h"
#include "openglprogram.h"
#include "options.h"

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

namespace {
const AbstractTrackTable *getTrackTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->trackTable(
      OpenDataFile::infoTable.getSelectedMontage());
}

const AbstractEventTable *getEventTable(OpenDataFile *file) {
  return file->dataModel->montageTable()->eventTable(
      OpenDataFile::infoTable.getSelectedMontage());
}

} // namespace

TfVisualizer::TfVisualizer(QWidget *parent) : QOpenGLWidget(parent) {
  //TODO: initiate all rectangles here and update later in paint
  specMesh = graphics::SquareMesh(-0.8f, 0.7f, -0.7f, 0.8f);
  
  gradient = std::make_unique<graphics::Gradient>(
    graphics::Gradient(0.75f, 0.8f, specMesh.getYbot(), specMesh.getYtop(), this));
}

TfVisualizer::~TfVisualizer() {
  if (glInitialized)
    cleanup();
	// Release these three objects here explicitly to make sure the right GL
	// context is bound by makeCurrent().
}

void TfVisualizer::deleteColormapTexture() {
  gl()->glDeleteTextures(1, &colormapTextureId);

  gl()->glBindTexture(GL_TEXTURE_1D, 0);
}

void TfVisualizer::cleanup() {
		makeCurrent();

    deleteColormapTexture();

		channelProgram.reset();

		logLastGLMessage();
		OPENGL_INTERFACE->checkGLErrors();

		doneCurrent();
}

GLuint TfVisualizer::setupColormapTexture(std::vector<float> colormap) {
  GLuint texId;
  gl()->glGenTextures(1, &texId);
  gl()->glBindTexture(GL_TEXTURE_1D, texId);
  //TODO: should this be here? corrupts text
  //gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gl()->glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, colormap.size() / 4, 0, GL_RGBA, GL_FLOAT, colormap.data());

  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl()->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLuint samplerLocation = gl()->glGetUniformLocation(channelProgram->getGLProgram(), "colormap");
  gl()->glUniform1i(samplerLocation, texId);

  return texId;
}

void TfVisualizer::updateColormapTexture() {
  gl()->glTexSubImage1D(GL_TEXTURE_1D, 0, 0, colormap.get().size() / 4, GL_RGBA, GL_FLOAT, colormap.get().data());
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

	QFile channelFragFile(":/channel.frag");
	channelFragFile.open(QIODevice::ReadOnly);
	string channelFrag = channelFragFile.readAll().toStdString();

	channelProgram = make_unique<OpenGLProgram>(triangleVert, channelFrag);

  gl()->glUseProgram(channelProgram->getGLProgram());

  colormapTextureId = setupColormapTexture(colormap.get());

  gl()->glActiveTexture(GL_TEXTURE0 + colormapTextureId);
  gl()->glBindTexture(GL_TEXTURE_1D, colormapTextureId);

  gl()->glFlush();

	createContext();
}

void TfVisualizer::resizeGL(int /*w*/, int /*h*/) {
  gradient->update();
}

//TODO: copy this to util class, and use it in scalpcanvas
//converts array to range while keeping the same ratio between elements
void TfVisualizer::convertToRange(std::vector<float>& values, float newMin, float newMax) {
  float minVal = FLT_MAX;
  float maxVal = FLT_MIN;

  for (int i = 0; i < values.size(); i++) {
    minVal = std::min(minVal, values[i]);
    maxVal = std::max(maxVal, values[i]);
  }

  float newRange = newMax - newMin;
  float oldRange = maxVal - minVal;

  for (int i = 0; i < values.size(); i++) {
    values[i] = (values[i] - minVal) * newRange / oldRange + newMin;
  }
  //std::cout << "CONVERTED VALUES: " << values[0] << "\n";
}

void TfVisualizer::setDataToDraw(std::vector<float> values, float xCount, float yCount) {
  //drawing points as squares in grid
  int xVertices = xCount + 1;
  int yVertices = yCount + 1;

  //std::cout << "setting data to draw\n";
  minGradVal = *std::min_element(values.begin(), values.end());
  maxGradVal = *std::max_element(values.begin(), values.end());

  convertToRange(values, 0.0f, 1.0f);

  //TODO: refactor this
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
    generateGradient(paintVertices, paintIndices);
  }
  else {
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
  }
}

void TfVisualizer::setSeconds(int secs) {
  seconds = secs;
}

void TfVisualizer::setFrequency(int fs) {
  frequency = fs;
}

//TODO: copied from scalpcanvas, move this to separate class
void TfVisualizer::generateGradient(std::vector<GLfloat>& triangles, std::vector<GLuint>& indices) {
  float gradientWidth = 0.05f;
  GLuint firstVertex = triangles.size() / 3;

  //1. triangle, left bot vertex
  triangles.push_back(gradient->getXleft());
  triangles.push_back(specMesh.getYbot());
  triangles.push_back(0.01f);
  //TODO: refactor this
  indices.push_back(firstVertex);

  //right bot vertex
  triangles.push_back(gradient->getXright());
  triangles.push_back(specMesh.getYbot());
  triangles.push_back(0.01f);
  indices.push_back(firstVertex + 1);

  //left top vertex
  triangles.push_back(gradient->getXleft());
  triangles.push_back(specMesh.getYtop());
  triangles.push_back(1);
  indices.push_back(firstVertex + 2);

  //2. triangle
  //right bot vertex
  indices.push_back(firstVertex + 1);

  //right top vertex
  triangles.push_back(gradient->getXright());
  triangles.push_back(specMesh.getYtop());
  triangles.push_back(1);
  indices.push_back(firstVertex + 3);

  // left top vertex
  indices.push_back(firstVertex + 2);
}

void TfVisualizer::paintGL() {
  using namespace chrono;

  if (paintingDisabled)
    return;

#ifndef NDEBUG
  logToFile("Painting started.");
#endif
  GLfloat red = palette().color(QPalette::Window).redF();
  GLfloat green = palette().color(QPalette::Window).greenF();
  GLfloat blue = palette().color(QPalette::Window).blueF();

  gl()->glClearColor(red, green, blue, 1.0f);

  auto specWindow = graphics::Rectangle(specMesh, this);
  specWindow.render();

  /*gradient.reset();
  gradient = std::make_unique<graphics::Gradient>(graphics::Gradient(gradientX, gradientX + 0.05f, specBotY, specTopY, this));*/

  /*auto gradWindow = graphics::Rectangle(gradientX, gradTopx, gradBoty, gradTopy, this);
  gradWindow.render();*/
  //std::cout << "gradWindow\n";
  auto gradWindow = graphics::Rectangle((graphics::Object) *gradient, this);
  gradWindow.render();

  auto gradAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(gradient->getXright(), gradient->getXright() + 0.02f, specMesh.getYbot(), specMesh.getYtop(), this, 5,
    graphics::Orientation::Vertical)
  );

  gradAxisLines->render();

  auto gradAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(gradient->getXright() + 0.05f, gradient->getXright() + 0.10f, specMesh.getYbot() + 0.025f,
    specMesh.getYtop() + 0.025f, this, 5, minGradVal, maxGradVal, QColor(0, 0, 0),
    graphics::Orientation::Vertical)
  );
  gradAxisNumbers->render();

  float specYSize = std::abs(specMesh.getYtop() - specMesh.getYbot());
  auto gradAxisLabel = graphics::RectangleText(0.95f, 0.98f,
    specMesh.getYbot() + specYSize / 3.0f, specMesh.getYtop() - specYSize / 3.0f, this, "Arial", QColor(0, 0, 0), "Amplitude",
    graphics::Orientation::Vertical, graphics::Orientation::Vertical);
  gradAxisLabel.render();

  //frequency
  
  /*graphics::Rectangle(-0.95f, -0.90f,
    specMesh.getYbot() + specYSize / 4, specMesh.getYtop() - specYSize / 4, this);
  frequencyAxisLabel.render();*/
  //TODO: specmesh getYbot seems off (or rotated text drawing is off)
  //specMesh.getYbot() + specYSize / 1.8f, specMesh.getYbot() + specYSize / 4.0
  auto frequencyAxisLabel = graphics::RectangleText(-0.98f, -0.92f,
    specMesh.getYbot() + specYSize / 3.0f, specMesh.getYtop() - specYSize / 3.0f, this, "Arial", QColor(0, 0, 0), "Frequency (Hz)",
    graphics::Orientation::Vertical, graphics::Orientation::Vertical);
  frequencyAxisLabel.render();

  auto frequencyAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(specMesh.getXleft() - 0.10f, specMesh.getXleft() - 0.05f, specMesh.getYbot() + 0.025f,
    specMesh.getYtop() + 0.025f, this, 5, 0, frequency, QColor(0, 0, 0), graphics::Orientation::Vertical)
  );
  frequencyAxisNumbers->render();
  
    auto frameAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(specMesh.getXleft() - 0.02f, specMesh.getXleft(), specMesh.getYbot(),
    specMesh.getYtop(), this, 5, graphics::Orientation::Vertical)
  );

  frameAxisLines->render();
  
  //time
  auto timeAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(specMesh.getXleft(), specMesh.getXright(), specMesh.getYbot() - 0.02f,
    specMesh.getYbot(), this, 5, graphics::Orientation::Horizontal, graphics::Orientation::Vertical)
  );

  timeAxisLines->render();

  auto timeAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(specMesh.getXleft() - 0.025f, specMesh.getXright() + 0.025f, specMesh.getYbot() - 0.15f,
    specMesh.getYbot() - 0.05f, this, 5, 0, seconds, QColor(0, 0, 0), graphics::Orientation::Horizontal,
    graphics::Orientation::Horizontal)
  );
  timeAxisNumbers->render();

  float specXSize = std::abs(specMesh.getXleft() - specMesh.getXright());
  auto timeAxisLabel = graphics::RectangleText(specMesh.getXleft() + specXSize / 2.4f,
    specMesh.getXright() - specXSize / 2.4f, specMesh.getYbot() - 0.25f, specMesh.getYbot() - 0.15f,
    this, "Arial", QColor(0, 0, 0), "Time(sec)", graphics::Orientation::Vertical,
    graphics::Orientation::Horizontal, graphics::Alignment::None);
  timeAxisLabel.render();

  if (ready()) {
    if (colormap.changed) {
      updateColormapTexture();
    }

    //std::cout << "tf painting\n";
    QPainter painter(this);
    painter.beginNativePainting();
    gl()->glUseProgram(channelProgram->getGLProgram());

    gl()->glGenBuffers(1, &indexBuffer);
    gl()->glGenBuffers(1, &posBuffer);

    gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
    gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    //TODO: static or dynamic draw?
    gl()->glBufferData(GL_ARRAY_BUFFER, paintVertices.size() * sizeof(GLfloat), &paintVertices[0], GL_STATIC_DRAW);

    // 1st attribute buffer : vertices
    //current position
    gl()->glEnableVertexAttribArray(0);
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void*)0);

    gl()->glEnableVertexAttribArray(1);
    gl()->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (char*)(sizeof(GLfloat) * 2));

    gl()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, paintIndices.size() * sizeof(GLuint), &paintIndices[0], GL_STATIC_DRAW);

    gl()->glDrawElements(GL_TRIANGLES, paintIndices.size(), GL_UNSIGNED_INT, nullptr);

    for (int i = 0; i < 2; i++) {
      gl()->glDisableVertexAttribArray(i);
    }

    //important for QPainter to work
    //gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
    //gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl()->glDeleteBuffers(1, &posBuffer);

    gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl()->glDeleteBuffers(1, &indexBuffer);

    gl()->glFlush();

    gl()->glFinish();

    painter.endNativePainting();
  }
}


bool TfVisualizer::ready() {
  return paintVertices.size();
}


void TfVisualizer::createContext() {

}


void TfVisualizer::logLastGLMessage() {

}

//TODO: dont call this when only freq gets changed, needs to be called when frameSize,
//      time and sampleCount(investigate how this changes with position change) is changed

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

  for (int i = 0; i < xAxis.size() - 1; i++) {
    for (int j = 0; j < yAxis.size() - 1; j++) {
      int valueIt = i * (yAxis.size() - 1) + j;
      GLuint squarePos = i * (yAxis.size() - 1) * 4 + j* 4;
      //create rectangle from 2 triangles
      //first triangle, left bot vertex
      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j]);
      //std::cout << "i: " << i << " j: " << j << " valueit: " << valueIt << " value size" << values.size() << "\n";
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
  } else if (event->button() == Qt::RightButton)
  {
    renderPopupMenu(event->pos());
  }
}
