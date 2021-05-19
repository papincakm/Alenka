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
  gradient = std::make_unique<graphics::Gradient>(graphics::Gradient(gradientX, gradientX + 0.05f, specBotY, specTopY, this));
}

TfVisualizer::~TfVisualizer() {
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

		gl()->glDeleteBuffers(1, &posBuffer);

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


void TfVisualizer::initializeGL() {
	logToFile("Initializing OpenGL in TfVisualizer.");

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

	gl()->glGenBuffers(1, &posBuffer);
	gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
	gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);

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
  posBufferData.clear();
  //std::cout << "setting data to draw\n";
  minGradVal = *std::min_element(values.begin(), values.end());
  maxGradVal = *std::max_element(values.begin(), values.end());

  //std::cout << "VALUES BEFORE: " << values[0] << "\n";
  convertToRange(values, 0.0f, 1.0f);
 // std::cout << "VALUES AFTER: " << values[0] << "\n";

  std::vector <float> xAxis = generateAxis(xCount);
  convertToRange(xAxis, specBotX, specTopX);

  std::vector<float> yAxis = generateAxis(yCount);
  convertToRange(yAxis, specBotY, specTopY);

  posBufferData = generateTriangulatedGrid(xAxis, yAxis, values);

  //gradient
  auto gradientBody = generateGradient();
  posBufferData.insert(std::end(posBufferData), std::begin(gradientBody), std::end(gradientBody));
}

void TfVisualizer::setSeconds(int secs) {
  seconds = secs;
}

void TfVisualizer::setFrameSize(int fs) {
  frameSize = fs;
}

//TODO: copied from scalpcanvas, move this to separate class
std::vector<GLfloat> TfVisualizer::generateGradient() {
  std::vector<GLfloat> triangles;

  float gradientWidth = 0.05f;

  //1. triangle
  triangles.push_back(gradientX);
  triangles.push_back(specBotY);
  triangles.push_back(0.01f);

  triangles.push_back(gradientX + gradientWidth);
  triangles.push_back(specBotY);
  triangles.push_back(0.01f);

  triangles.push_back(gradientX);
  triangles.push_back(specTopY);
  triangles.push_back(1);

  //2. triangle
  triangles.push_back(gradientX + gradientWidth);
  triangles.push_back(specBotY);
  triangles.push_back(0.01f);

  triangles.push_back(gradientX + gradientWidth);
  triangles.push_back(specTopY);
  triangles.push_back(1);

  triangles.push_back(gradientX);
  triangles.push_back(specTopY);
  triangles.push_back(1);

  return triangles;
}

void TfVisualizer::paintGL() {
  using namespace chrono;

  if (paintingDisabled)
    return;

#ifndef NDEBUG
  logToFile("Painting started.");
  GLfloat red = palette().color(QPalette::Window).redF();
  GLfloat green = palette().color(QPalette::Window).greenF();
  GLfloat blue = palette().color(QPalette::Window).blueF();

  gl()->glClearColor(red, green, blue, 1.0f);

  auto specWindow = graphics::Rectangle(specBotX, specTopX, specBotY, specTopY, this);
  specWindow.render();

  float gradTopx = gradientX + 0.05f + 0.003f;
  float gradBoty = specBotY - 0.003f;
  float gradTopy = specTopY + 0.003f;

  /*gradient.reset();
  gradient = std::make_unique<graphics::Gradient>(graphics::Gradient(gradientX, gradientX + 0.05f, specBotY, specTopY, this));*/

  /*auto gradWindow = graphics::Rectangle(gradientX, gradTopx, gradBoty, gradTopy, this);
  gradWindow.render();*/
  std::cout << "gradWindow\n";
  auto gradWindow = graphics::Rectangle(gradientX, gradientX + 0.05f, specBotY, specTopY, this);
  gradWindow.render();

  auto gradAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(gradientX - 0.02f, gradientX, specBotY, specTopY, this, 5, graphics::Orientation::Vertical)
  );

  gradAxisLines->render();

  auto gradAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(gradientX - 0.15f, gradientX - 0.05f, specBotY + 0.025f, specTopY + 0.025f, this, 5, minGradVal, maxGradVal,
      QColor(0, 0, 0), graphics::Orientation::Vertical)
  );
  gradAxisNumbers->render();

  auto frameAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(specBotX - 0.02f, specBotX, specBotY, specTopY, this, 5, graphics::Orientation::Vertical));

  frameAxisLines->render();

  auto frameAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(specBotX - 0.15f, specBotX - 0.05f, specBotY + 0.025f, specTopY + 0.025f, this, 5, 0, frameSize,
      QColor(0, 0, 0), graphics::Orientation::Vertical)
  );
  frameAxisNumbers->render();

  auto timeAxisLines = graphics::RectangleChainFactory<graphics::LineChain>().make(
    graphics::LineChain(specBotX, specTopX, specBotY - 0.02f, specBotY, this, 5, 
      graphics::Orientation::Horizontal, graphics::Orientation::Vertical));

  timeAxisLines->render();

  auto timeAxisNumbers = graphics::RectangleChainFactory<graphics::NumberRange>().make(
    graphics::NumberRange(specBotX + 0.025f, specTopX + 0.025f, specBotY - 0.15f, specBotY - 0.05f, this, 5, 0, seconds,
      QColor(0, 0, 0), graphics::Orientation::Horizontal, graphics::Orientation::Horizontal)
  );
  timeAxisNumbers->render();

#endif
  if (ready()) {
    std::cout << "tf painting\n";
    QPainter painter(this);
    painter.beginNativePainting();
    gl()->glUseProgram(channelProgram->getGLProgram());

    gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

    //TODO: static or dynamic draw?
    gl()->glBufferData(GL_ARRAY_BUFFER, posBufferData.size() * sizeof(GLfloat), &posBufferData[0], GL_STATIC_DRAW);

    // 1st attribute buffer : vertices
    //current position
    gl()->glEnableVertexAttribArray(0);
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void*)0);

    gl()->glEnableVertexAttribArray(1);
    gl()->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (char*)(sizeof(GLfloat) * 2));

    gl()->glDrawArrays(GL_TRIANGLES, 0, posBufferData.size());

    for (int i = 0; i < 2; i++) {
      gl()->glDisableVertexAttribArray(i);
    }

    //important for QPainter to work
    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gl()->glFlush();

    gl()->glFinish();

    painter.endNativePainting();
  }
}


bool TfVisualizer::ready() {
  return posBufferData.size();
}


void TfVisualizer::createContext() {

}


void TfVisualizer::logLastGLMessage() {

}

//TODO: dont call this when only freq gets changed, needs to be called when frameSize,
//      time and sampleCount(investigate how this changes with position change) is changed

std::vector<float> TfVisualizer::generateAxis(int pointCount) {
  std::vector<float> axis;
  float pc = pointCount;
  for (int i = 0; i < pointCount; i++) {
    axis.push_back(i / pc);
  }
  return axis;
}

std::vector<GLfloat> TfVisualizer::generateTriangulatedGrid(const std::vector<float> xAxis,
  const std::vector<float> yAxis, const std::vector<float>& values) {
  
  std::vector<GLfloat> triangles;

  for (int i = 0; i < xAxis.size() - 1; i++) {
    for (int j = 0; j < yAxis.size() - 1; j++) {
      int valueIt = i * yAxis.size() + j;
      //create rectangle from 2 triangles
      //first triangle
      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j]);
      //std::cout << "i: " << i << " j: " << j << " valueit: " << valueIt << " value size" << values.size() << "\n";
      triangles.push_back(values[valueIt]);

      triangles.push_back(xAxis[i + 1]);
      triangles.push_back(yAxis[j]);
      triangles.push_back(values[valueIt]);

      triangles.push_back(xAxis[i + 1]);
      triangles.push_back(yAxis[j + 1]);
      triangles.push_back(values[valueIt]);

      //second triangle
      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j]);
      triangles.push_back(values[valueIt]);

      triangles.push_back(xAxis[i]);
      triangles.push_back(yAxis[j + 1]);
      triangles.push_back(values[valueIt]);

      triangles.push_back(xAxis[i + 1]);
      triangles.push_back(yAxis[j + 1]);
      triangles.push_back(values[valueIt]);
    }
  }

  return triangles;
}

void TfVisualizer::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && gradient->contains(event->pos())) {
    gradient->clicked(event->pos());
  }
}

void TfVisualizer::mouseMoveEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->change(colormap, event->globalPos());
    makeCurrent();
    gl()->glTexSubImage1D(GL_TEXTURE_1D, 0, 0, colormap.get().size() / 4, GL_RGBA, GL_FLOAT, colormap.get().data());
    doneCurrent();
    update();
  }
}

void TfVisualizer::mouseReleaseEvent(QMouseEvent* event) {
  if (gradient->isClicked) {
    gradient->released();
  }
}
