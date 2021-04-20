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
}

TfVisualizer::~TfVisualizer() {
  cleanup();
	// Release these three objects here explicitly to make sure the right GL
	// context is bound by makeCurrent().
}

void TfVisualizer::cleanup() {
		makeCurrent();

		gl()->glDeleteBuffers(1, &posBuffer);

    gl()->glDeleteTextures(1, &colormapTextureId);

    gl()->glBindTexture(GL_TEXTURE_1D, 0);

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
  // checkGLMessages();
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
  auto gradient = generateGradient();
  posBufferData.insert(std::end(posBufferData), std::begin(gradient), std::end(gradient));
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

//TODO: copied from scalpcanvas
void TfVisualizer::renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor) {
  int realX = width() / 2 + (width() / 2) * x;
  int realY = height() / 2 + (height() / 2) * y * -1;

  //text is corrupted if this is not called
  //gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  QPainter painter(this);
  painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.setRenderHints(QPainter::TextAntialiasing);
  painter.setBackground(QBrush(QColor(0, 0, 0)));
  painter.setPen(fontColor);
  painter.setBrush(fontColor);
  painter.setFont(font);
  painter.drawText(x, y, str);
  painter.end();
}

//TODO: move to separate draw utils class
void TfVisualizer::renderVertical(const GraphicsNumberRange& range, QColor color) {
  QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);

  float maxMinusMinY = range.topY - range.botY;
  float maxMinusMinNum = range.to - range.from;

  float binY = maxMinusMinY / (range.numberCount - 1);
  float binNum = maxMinusMinNum / (range.numberCount - 1);

  float numToDisplay = range.from;
  float yPos = range.botY;

  for (int i = 0; i < range.numberCount; i++) {
    //TODO: tie this to font height somehow
    if (i == (range.numberCount - 1))
      yPos -= 0.065f;

    renderText(range.botX, yPos, QString::number(numToDisplay, 'f', 3), gradientNumberFont, color);
    numToDisplay += binNum;
    yPos += binY;
  }
}

void TfVisualizer::renderHorizontal(const GraphicsNumberRange& range, QColor color) {
  QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);

  float maxMinusMinX = range.topX - range.botX;
  float maxMinusMinNum = range.to - range.from;

  float binX = maxMinusMinX / (range.numberCount - 1);
  float binNum = maxMinusMinNum / (range.numberCount - 1);

  float numToDisplay = range.from;
  float xPos = range.botX;

  for (int i = 0; i < range.numberCount; i++) {
    //TODO: tie this to font height somehow
    if (i == (range.numberCount - 1))
      xPos -= 0.065f;

    renderText(xPos, range.botY, QString::number(numToDisplay, 'f', 1), gradientNumberFont, color);
    numToDisplay += binNum;
    xPos += binX;
  }
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

  auto specWindow = graphics::Rectangle(specBotX, specTopX + 0.006f, specBotY - 0.005f, specTopY + 0.005, this);
  specWindow.render();

  auto gradWindow = graphics::Rectangle(gradientX, gradientX + 0.05f + 0.006f, specBotY - 0.005f, specTopY + 0.005, this);
  gradWindow.render();

#endif
  if (ready()) {

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

    //gradientText
    //GraphicsNumberRange gradientNumbers(minGradVal, maxGradVal, 5, 0.7f, specBotY, specTopX, specTopY);
    //renderVertical(gradientNumbers, QColor(255, 255, 255));
    //frequency text
    //GraphicsNumberRange frequencyNumbers(0, frameSize, 5, -1, specBotY, specTopX, specTopY);
    //renderVertical(frequencyNumbers, QColor(255, 255, 255));
    //time text
    /*GraphicsNumberRange timeNumbers(0, seconds, 5, specBotX, specBotY - 0.05f, specTopX, specTopY);
    renderHorizontal(timeNumbers, QColor(255, 255, 255));*/
    //GraphicsNumberRange timeNumbers(0, seconds, 5, specBotX, specTopY, specTopX, specBotY - 0.05f);
    //renderVertical(timeNumbers, QColor(255, 255, 255));
    //renderText(50, 100, "HAHAHAHAHAHAHAH", QFont("Arial", 13, QFont::Bold), QColor(1, 0, 0));
    auto gradientNumbers = 
      graphics::NumberRange(0.75f, 0.9f, -0.7f, 0.8f, this, 5, minGradVal, maxGradVal, QColor(0, 0, 0), graphics::Vertical);

    gradientNumbers.render();

    auto frameNumbers =
      graphics::NumberRange(-1, specTopX, specBotY, specTopY, this, 5, 0, frameSize, QColor(0, 0, 0), graphics::Vertical);

    frameNumbers.render();

    auto timeNumbers = 
      graphics::NumberRange(specBotX, specTopX, specBotY - 0.20f, specBotY - 0.05f, this, 5, 0, seconds, QColor(0, 0, 0), graphics::Horizontal);

    timeNumbers.render();

    //auto rec = graphics::RectangleText(-0.9f, -0.9f, -0.7f, -0.7f, this, "Arial", QColor(1, 0, 0), "HAHAHAAHAHAH");
    //rec.render();

    gl()->glFinish();
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