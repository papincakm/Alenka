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

using namespace std;
using namespace AlenkaFile;
using namespace AlenkaSignal;

ScalpCanvas::ScalpCanvas(QWidget *parent) : QOpenGLWidget(parent) {
  //std::cout << "scalpcanvas constructor\n";
  gradient = std::make_unique<graphics::Gradient>(
    graphics::Gradient(gradientX, gradientX + 0.05f, gradientBotY, gradientTopY, this));
}

ScalpCanvas::~ScalpCanvas() {
	logToFile("Destructor in ScalpCanvas.");
	if (glInitialized)
    cleanup();

  channelProgram.reset();

  labelProgram.reset();
}

void ScalpCanvas::forbidDraw(QString errorString) {
		errorMsg = errorString;
		dataReadyToDraw = false;
}

void ScalpCanvas::allowDraw() {
		errorMsg = "";
		dataReadyToDraw = true;
}

void ScalpCanvas::clear() {
		originalPositions.clear();
		triangulatedPositions.clear();

		update();
}

void bindArray(GLuint array, GLuint buffer) {
	if (programOption<bool>("gl20")) {
		gl()->glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}
	else {
		gl3()->glBindVertexArray(array);
	}
}

void ScalpCanvas::initializeGL() {
  //std::cout << "scalpcanvas initialized\n";
  glInitialized = true;
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScalpCanvas::cleanup);

	if (!OPENGL_INTERFACE) {
		OPENGL_INTERFACE = make_unique<OpenGLInterface>();
		OPENGL_INTERFACE->initializeOpenGLInterface();
	}

	gl()->glEnable(GL_PROGRAM_POINT_SIZE);
	gl()->glEnable(GL_POINT_SPRITE);
	
	//circle version
	//TODO: rename shaders to something more meaningful

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

	QFile channelFragFile(":/channel.frag");
	channelFragFile.open(QIODevice::ReadOnly);
	string channelFrag = channelFragFile.readAll().toStdString();

	channelProgram = make_unique<OpenGLProgram>(triangleVert, channelFrag);

  gl()->glUseProgram(channelProgram->getGLProgram());
	//gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

  colormapTextureId = setupColormapTexture(colormap.get());

  gl()->glActiveTexture(GL_TEXTURE0 + colormapTextureId);
  gl()->glBindTexture(GL_TEXTURE_1D, colormapTextureId);

  gl()->glFlush();

  checkGLMessages();
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

  menu->addSeparator();

  //setup extrema
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
    // std::cout << "MENU: LOCAL SELECTED\n";
    setExtremaLocal.setChecked(true);
    break;
  }

  menu->addMenu(extremaMenu);

  menu->exec(mapToGlobal(pos));
}
//TODO: use indices for painting
void ScalpCanvas::cleanup() {
	logToFile("Cleanup in ScalpCanvas.");
	makeCurrent();

  gl()->glDeleteTextures(1, &colormapTextureId);
  gl()->glBindTexture(GL_TEXTURE_1D, 0);

	channelProgram.reset();

	labelProgram.reset();

	logLastGLMessage();
	OPENGL_INTERFACE->checkGLErrors();

	doneCurrent();
}

std::vector<float> generateAxis(int pointCount) {
  std::vector<float> axis;
  float pc = pointCount;
  for (int i = 0; i < pointCount; i++) {
    axis.push_back(i / pc);
  }
  return axis;
}

//TODO: copy this to util class, and use it in scalpcanvas
//converts array to range while keeping the same ratio between elements
void convertToRange(std::vector<float>& values, float newMin, float newMax) {
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

void ScalpCanvas::setupScalpMesh() {
  /*float specBotX = -0.8f;
  float specTopX = 0.7f;
  float specBotY = -0.7f;
  float specTopY = 0.8f;
  int xLen = 100;
  int yLen = 100;

  std::vector<float> xAxis = generateAxis(xLen);
  convertToRange(xAxis, specBotX, specTopX);

  std::vector<float> yAxis = generateAxis(yLen);
  convertToRange(yAxis, specBotY, specTopY);
  int valCnt = xLen * yLen;
  std::vector<float> values(valCnt, 0);

  scalpMesh = generateTriangulatedGrid(xAxis, yAxis, values);*/
  scalpMesh = generateScalpTriangleArray();
  calculateSpatialCoefficients(scalpMesh);
  calculateVoltages(scalpMesh);

  std::vector<GLfloat> gradient = generateGradient();
  scalpMesh.insert(std::end(scalpMesh), std::begin(gradient), std::end(gradient));
}

void ScalpCanvas::setChannelPositions(const std::vector<QVector2D>& channelPositions) {
  originalPositions.clear();
  triangulatedPositions.clear();
  scalpMesh.clear();

  //TODO: what if positions are not always rotated the same way
  //TODO: make rotation option for user
  for (auto v : channelPositions) {
    //TODO: multiply more transparently, use different method, find proper way to do this
    originalPositions.push_back(ElectrodePosition(-1 * v.y() - 0.17f, v.x()));
  }

  //TODO: refactor me
  triangulatedPositions = generateTriangulatedPositions(originalPositions);
  //std::cout << "setup scalp mesh from set channel pos\n";
  setupScalpMesh();
}

void ScalpCanvas::setChannelLabels(const std::vector<QString>& channelLabels) {
  labels = channelLabels;
}

void ScalpCanvas::setPositionVoltages(const std::vector<float>& channelDataBuffer, const float& min, const float& max) {
	//TODO: theres less positions thant channelDataBuffer
	//std::cout << "positions: " << positions.size() << "  channelBuffer: " << channelDataBuffer.size() << "freqs:\n";
	//TODO: investigate, positions are sometimes smaller, even thoug scalpmap should take care of this
	if (static_cast<int>(originalPositions.size()) < static_cast<int>(channelDataBuffer.size()))
		return;
	
	minVoltage = min;
	maxVoltage = max;

  //TODO: if i set custom minmax to 1000 and 1500 (or -1000 2500) then some triangles are blue and should be red
	float maxMinusMin = maxVoltage - minVoltage;

	int size = std::min(originalPositions.size(), channelDataBuffer.size());

	for (int i = 0; i < size; i++) {
    float newVoltage = (channelDataBuffer[i] - minVoltage) / (maxMinusMin);
		originalPositions[i].voltage = newVoltage < 0 ? 0 : (newVoltage > 1 ? 1 : newVoltage);
	}

  calculateVoltages(scalpMesh);
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
  gradient->update();
}

//TODO: testing, refactor in future, use less data containers
std::vector<ElectrodePosition> ScalpCanvas::generateTriangulatedPositions(const std::vector<ElectrodePosition>& channels) {
	std::vector<double> coords;
	std::vector<ElectrodePosition> triangles;
	
	//TODO: do this is setChannelPositions and skip positions?
	for (auto ch : channels) {
		coords.push_back(ch.x);
		coords.push_back(ch.y);
	}
	//TODO: vector out of bound, when montage changed(which has all 0 as coordinates - might be the reason)
	delaunator::Delaunator d(coords);
	
	for (std::size_t i = 0; i < d.triangles.size(); i+=3) {
		triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i]],
											  d.coords[2 * d.triangles[i] + 1]));
		triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i + 1]],
											  d.coords[2 * d.triangles[i + 1] + 1]));
		triangles.push_back(ElectrodePosition(d.coords[2 * d.triangles[i + 2]],
											  d.coords[2 * d.triangles[i + 2] + 1]));
	}

	return triangles;
}

float getDistance(const float x1, const float y1, const float x2, const float y2) {
  return std::sqrt((x1 - x2) * (x1 - x2) +
    (y1 - y2) * (y1- y2));
}
//takes triangulated positions, whe each position is represented by 3 floats - x, y, z
void ScalpCanvas::calculateSpatialCoefficients(const std::vector<GLfloat>& points) {
  pointSpatialCoefficients.clear();

  for (int i = 0; i < points.size(); i += 3) {
    std::vector<std::pair<float, int>> distances;
    std::vector<PointSpatialCoefficient> singlePointSpatialCoefficients;
    float sumDistance = 0;
    bool samePoint = false;

    for (int j = 0; j < originalPositions.size(); j++) {
      float distance = getDistance(points[i], points[i + 1], originalPositions[j].x, originalPositions[j].y);

      if (distance == 0) {
        distance = 0.00001f;
      }

      distances.push_back(std::make_pair(1.0f / (distance * distance), j));
    }

    std::sort(distances.begin(), distances.end());

    float newFrequency = 0;

    int usedNearestPos = 3;

    for (int j = distances.size() - 1; j > distances.size() - usedNearestPos - 1; j--) {
      singlePointSpatialCoefficients.push_back(PointSpatialCoefficient(1 / distances[j].first * distances[j].first * distances[j].first, distances[j].second));
    }

    pointSpatialCoefficients.push_back(singlePointSpatialCoefficients);
  }
}
void ScalpCanvas::calculateVoltages(std::vector<GLfloat>& points) {
  for (int i = 0; i < pointSpatialCoefficients.size(); i ++) {
    float newVoltage = 0;
    float sumCoefficient = 0;
    for (int j = 0; j < pointSpatialCoefficients[i].size(); j++) {
      newVoltage += pointSpatialCoefficients[i][j].coefficient * originalPositions[pointSpatialCoefficients[i][j].toPoint].voltage;
      sumCoefficient += pointSpatialCoefficients[i][j].coefficient;
    }
    newVoltage /= sumCoefficient;

    points[i * 3 + 2] = newVoltage;
  }
}

std::vector<GLfloat> ScalpCanvas::generateScalpTriangleArray() {
	std::vector<GLfloat> triangles;

	//TODO: refactor with single struct
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		//current position
		//TODO: can this be avoided?
		triangles.push_back(triangulatedPositions[i].x);
		triangles.push_back(triangulatedPositions[i].y);

		//frequency
		triangles.push_back(triangulatedPositions[i].voltage);
	}

  auto ss = splitTriangles(triangles);
  //ss = splitTriangles(ss);
  auto finalTriangles = splitTriangles(ss);

	return finalTriangles;
}

//TODO: might want to separate frequencies and vertices
std::vector<GLfloat> ScalpCanvas::generateGradient() {
	std::vector<GLfloat> triangles;

	float gradientWidth = 0.05f;

	//1. triangle
	triangles.push_back(gradientX);
	triangles.push_back(gradientBotY);
	triangles.push_back(0.01f);

	triangles.push_back(gradientX + gradientWidth);
	triangles.push_back(gradientBotY);
	triangles.push_back(0.01f);

	triangles.push_back(gradientX);
	triangles.push_back(gradientTopY);
	triangles.push_back(1);

	//2. triangle
	triangles.push_back(gradientX + gradientWidth);
	triangles.push_back(gradientBotY);
	triangles.push_back(0.01f);

	triangles.push_back(gradientX + gradientWidth);
	triangles.push_back(gradientTopY);
	triangles.push_back(1);

	triangles.push_back(gradientX);
	triangles.push_back(gradientTopY);
	triangles.push_back(1);

	return triangles;
}

void ScalpCanvas::renderGradientText() {
  QFont gradientNumberFont = QFont("Times", 15, QFont::Bold);
	
	float maxMinusMinY = gradientTopY - gradientBotY;
	float maxMinusMinFreq = maxVoltage - minVoltage;

	float binY = maxMinusMinY / 4;
	float binFreq = maxMinusMinFreq / 4;

	float freqToDisplay = minVoltage;
	float yPos = gradientBotY;

	for (int i = 0; i < 5; i += 1) {
		//TODO: tie this to font height somehow
		if (i == 4)
			yPos -= 0.065;

		renderText(0.7f, yPos, QString::number(((int) freqToDisplay)), gradientNumberFont, QColor(255, 255, 255));
		freqToDisplay += binFreq;
		yPos += binY;
	}
}

GLuint ScalpCanvas::setupColormapTexture(std::vector<float> colormap) {
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

void ScalpCanvas::updateColormapTexture() {
  gl()->glTexSubImage1D(GL_TEXTURE_1D, 0, 0, colormap.get().size() / 4, GL_RGBA, GL_FLOAT, colormap.get().data());
  colormap.changed = false;
}

//TODO: doesnt refresh on table change
void ScalpCanvas::paintGL() {
	using namespace chrono;

#ifndef NDEBUG
	logToFile("Painting started.");
#endif
	if (ready()) {
    //setup
    if (colormap.changed) {
      updateColormapTexture();
    }

    if (scalpMesh.empty()) {
      //std::cout << "setup scalpMesh from paint event\n";
      setupScalpMesh();
    }

    gl()->glUseProgram(channelProgram->getGLProgram());

    gl()->glGenBuffers(1, &posBuffer);
    gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

    gl()->glBufferData(GL_ARRAY_BUFFER, scalpMesh.size() * sizeof(GLfloat), &scalpMesh[0], GL_STATIC_DRAW);

    // 1st attribute buffer : vertices
    //current position
    gl()->glEnableVertexAttribArray(0);
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void*)0);

    gl()->glEnableVertexAttribArray(1);
    gl()->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (char*)(sizeof(GLfloat) * 2));


    gl()->glDrawArrays(GL_TRIANGLES, 0, scalpMesh.size());

    for (int i = 0; i < 2; i++) {
      gl()->glDisableVertexAttribArray(i);
    }

    gl()->glFlush();

    // draw channels TODO: refactor - dont use points, add to main channel
    // TODO!!!:draw once and set it to be top of screen so it cant be drawn over
    //
    //
    //POINTS
    //TODO: use positions, triangulatedPositions are inflated, repeated draws
    if (shouldDrawChannels) {
      gl()->glPointSize(10.0f);
      std:vector<GLfloat> channelBufferData(originalPositions.size() * 2);
      for (int i = 0; i < originalPositions.size(); i++) {
        channelBufferData.push_back(originalPositions[i].x);
        channelBufferData.push_back(originalPositions[i].y);
      }
      gl()->glUseProgram(labelProgram->getGLProgram());
      gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

      gl()->glBufferData(GL_ARRAY_BUFFER, channelBufferData.size() * sizeof(GLfloat), &channelBufferData[0], GL_STATIC_DRAW);

      gl()->glEnableVertexAttribArray(0);
      gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
      gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
      gl()->glDrawArrays(GL_POINTS, 0, channelBufferData.size());
      gl()->glDisableVertexAttribArray(0);
    }

    //important for QPainter to work
    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gl()->glFlush();

    if (shouldDrawLabels) {
      for (int i = 0; i < originalPositions.size(); i++) {
        QFont labelFont = QFont("Times", 8, QFont::Bold);

        renderText(originalPositions[i].x, originalPositions[i].y, labels[i], labelFont, QColor(255, 255, 255));
      }
    }

    //QPainter part
    renderGradientText();

    gl()->glFinish();

    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl()->glDeleteBuffers(1, &posBuffer);
  }

	if (errorMsg != "")
			renderErrorMsg();

  checkGLMessages();

#ifndef NDEBUG
	logToFile("Painting finished.");
#endif
}

void ScalpCanvas::renderErrorMsg() {
		auto errorFont = QFont("Times", 15, QFont::Bold);
		renderText(-0.8f, 0, errorMsg, errorFont, QColor(255, 0, 0));
}

float ScalpCanvas::virtualRatio() {
	return 0;
}

float ScalpCanvas::leftEdgePosition() {
  return OpenDataFile::infoTable.getPosition() -
         OpenDataFile::infoTable.getPositionIndicator() * width() *
             virtualRatio();
}

void ScalpCanvas::logLastGLMessage() {
  if (lastGLMessageCount > 1) {
    logToFile("OpenGL message (" << lastGLMessageCount - 1 << "x): " << lastGLMessage);
  }
}

bool ScalpCanvas::ready() {
	//TODO: think this through
  //std::cout << "should draw" << dataReadyToDraw << "triangulatedPos size: " << triangulatedPositions.size() << "\n";
  return dataReadyToDraw && triangulatedPositions.size() > 0;
}

void ScalpCanvas::renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor) {
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
	painter.drawText(realX, realY, str);
  painter.end();
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

//TODO: refactor with triangle class and operations!!
//TODO: consider using geometry shader
std::vector<GLfloat> ScalpCanvas::splitTriangles(const std::vector<GLfloat>& triangles) {
  assert(static_cast<int>(triangles.size()) % 3 == 0);

  std::vector<GLfloat> splitTriangles;

  for (int i = 0; i < triangles.size(); i += 9) {
    const int vertexOffset = 3;
    const int yOffset = 1;
    const int freqOffset = 2;

    //1. vertex, 2. vertex
    float midPointAx = 0.5f * triangles[i] + 0.5f * triangles[i + vertexOffset];
    float midPointAy = 0.5f * triangles[i + yOffset] + 0.5f * triangles[i + vertexOffset + yOffset];
    float midPointAfreq = 0.5f * triangles[i + freqOffset] + 0.5f * triangles[i + vertexOffset + freqOffset];

    //1. vertex, 3. vertex
    float midPointBx = 0.5f * triangles[i] + 0.5f * triangles[i + vertexOffset * 2];
    float midPointBy = 0.5f * triangles[i + yOffset] + 0.5f * triangles[i + vertexOffset * 2 + yOffset];
    float midPointBfreq = 0.5f * triangles[i + freqOffset] + 0.5f * triangles[i + vertexOffset * 2 + freqOffset];

    //2. vertex, 3. vertex
    float midPointCx = 0.5f * triangles[i + vertexOffset] + 0.5f * triangles[i + vertexOffset * 2];
    float midPointCy = 0.5f * triangles[i + vertexOffset + yOffset] + 0.5f * triangles[i + vertexOffset * 2 + yOffset];
    float midPointCfreq = 0.5f * triangles[i + vertexOffset + freqOffset] + 0.5f * triangles[i + vertexOffset * 2 + freqOffset];

    //1. triangle
    splitTriangles.push_back(triangles[i]);
    splitTriangles.push_back(triangles[i + yOffset]);
    splitTriangles.push_back(triangles[i + freqOffset]);

    splitTriangles.push_back(midPointAx);
    splitTriangles.push_back(midPointAy);
    splitTriangles.push_back(midPointAfreq);

    splitTriangles.push_back(midPointBx);
    splitTriangles.push_back(midPointBy);
    splitTriangles.push_back(midPointBfreq);

    //2. triangle
    splitTriangles.push_back(midPointAx);
    splitTriangles.push_back(midPointAy);
    splitTriangles.push_back(midPointAfreq);

    splitTriangles.push_back(midPointBx);
    splitTriangles.push_back(midPointBy);
    splitTriangles.push_back(midPointBfreq);

    splitTriangles.push_back(midPointCx);
    splitTriangles.push_back(midPointCy);
    splitTriangles.push_back(midPointCfreq);

    //3. triangle
    splitTriangles.push_back(triangles[i + vertexOffset]);
    splitTriangles.push_back(triangles[i + vertexOffset + yOffset]);
    splitTriangles.push_back(triangles[i + vertexOffset + freqOffset]);

    splitTriangles.push_back(midPointAx);
    splitTriangles.push_back(midPointAy);
    splitTriangles.push_back(midPointAfreq);

    splitTriangles.push_back(midPointCx);
    splitTriangles.push_back(midPointCy);
    splitTriangles.push_back(midPointCfreq);

    //4. triangle
    splitTriangles.push_back(triangles[i + vertexOffset * 2]);
    splitTriangles.push_back(triangles[i + vertexOffset * 2 + yOffset]);
    splitTriangles.push_back(triangles[i + vertexOffset * 2 + freqOffset]);

    splitTriangles.push_back(midPointBx);
    splitTriangles.push_back(midPointBy);
    splitTriangles.push_back(midPointBfreq);

    splitTriangles.push_back(midPointCx);
    splitTriangles.push_back(midPointCy);
    splitTriangles.push_back(midPointCfreq);
  }

  assert(static_cast<int>(splitTriangles.size()) % 3 == 0);

  return splitTriangles;
}

std::vector<GLfloat> ScalpCanvas::generateTriangulatedGrid(const std::vector<float> xAxis,
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