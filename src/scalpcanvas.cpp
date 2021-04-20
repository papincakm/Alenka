#include "scalpcanvas.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "DataModel/opendatafile.h"
#include "DataModel/undocommandfactory.h"
#include "DataModel/vitnessdatamodel.h"
#include "SignalProcessor/signalprocessor.h"
#include "error.h"
#include "myapplication.h"
#include "openglprogram.h"
#include "options.h"
#include "signalviewer.h"

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
}

ScalpCanvas::~ScalpCanvas() {
	logToFile("Destructor in ScalpCanvas.");
	cleanup();
}

void ScalpCanvas::forbidDraw(QString errorString) {
		errorMsg = errorString;
		shouldDraw = false;
}

void ScalpCanvas::allowDraw() {
		errorMsg = "";
		shouldDraw = true;
}

void ScalpCanvas::clear() {
		originalPositions.clear();
		triangulatedPositions.clear();

		update();
}

void ScalpCanvas::setChannelPositions(const std::vector<QVector2D>& channelPositions) {
	originalPositions.clear();
	triangulatedPositions.clear();
	
	//TODO: what if positions are not always rotated the same way
	//TODO: make rotation option for user
	for (auto v : channelPositions) {
		//TODO: multiply more transparently, use different method, find proper way to do this
		originalPositions.push_back(ElectrodePosition(-1 * v.y(), v.x()));
	}
	
	//TODO: refactor me
	triangulatedPositions = generateTriangulatedPositions(originalPositions);
}

void ScalpCanvas::setChannelLabels(const std::vector<QString>& channelLabels) {
	labels = channelLabels;
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

	gl()->glGenBuffers(1, &posBuffer);
	//gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

  colormapTextureId = setupColormapTexture(colormap.get());

  gl()->glActiveTexture(GL_TEXTURE0 + colormapTextureId);
  gl()->glBindTexture(GL_TEXTURE_1D, colormapTextureId);

  gl()->glFlush();
}

void ScalpCanvas::mouseReleaseEvent(QMouseEvent * event) {
  if (event->button() == Qt::RightButton)
  {
    //TODO: Move this to separate file
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

    //set global extrema
    QAction setExtremaGlobal("Global", this);

    connect(&setExtremaGlobal, &QAction::triggered, [this]() {
      OpenDataFile::infoTable.setExtremaGlobal();
    });
    setExtremaGlobal.setCheckable(true);

    extremaMenu->addAction(&setExtremaGlobal);
    extremaGroup->addAction(&setExtremaGlobal);

    //set custom extrema
    /*QAction setExtremaCustom("Custom", this);

    connect(&setExtremaCustom, &QAction::triggered, [this]() {
      OpenDataFile::infoTable.setExtremaCustom();
    });
    setExtremaCustom.setCheckable(true);*/

    //extremaMenu->addAction(&setExtremaCustom);
    //extremaGroup->addAction(&setExtremaCustom);

    switch (OpenDataFile::infoTable.getScalpMapExtrema()) {
    /*case InfoTable::Extrema::custom:
      setExtremaCustom.setChecked(true);
      break;*/
    case InfoTable::Extrema::global:
     // std::cout << "MENU: GLOBAL SELECTED\n";
      setExtremaGlobal.setChecked(true);
      break;
    case InfoTable::Extrema::local:
     // std::cout << "MENU: LOCAL SELECTED\n";
      setExtremaLocal.setChecked(true);
      break;
    }
 
    menu->addMenu(extremaMenu);

    menu->exec(mapToGlobal(event->pos()));
  }
  QOpenGLWidget::mouseReleaseEvent(event);
}

void ScalpCanvas::cleanup() {
	logToFile("Cleanup in ScalpCanvas.");
	makeCurrent();

	gl()->glDeleteBuffers(1, &posBuffer);

  gl()->glDeleteTextures(1, &colormapTextureId);

  gl()->glBindTexture(GL_TEXTURE_1D, 0);

	posBufferData.clear();

	channelProgram.reset();

	labelProgram.reset();

	logLastGLMessage();
	OPENGL_INTERFACE->checkGLErrors();

	doneCurrent();
}

void ScalpCanvas::setPositionFrequencies(const std::vector<float>& channelDataBuffer, const float& min, const float& max) {
	//TODO: theres less positions thant channelDataBuffer
	//std::cout << "positions: " << positions.size() << "  channelBuffer: " << channelDataBuffer.size() << "freqs:\n";
	//TODO: investigate, positions are sometimes smaller, even thoug scalpmap should take care of this
	if (static_cast<int>(originalPositions.size()) < static_cast<int>(channelDataBuffer.size()))
		return;
	
	minFrequency = min;
	maxFrequency = max;

	float maxMinusMin = maxFrequency - minFrequency;

	int size = std::min(originalPositions.size(), channelDataBuffer.size());

	for (int i = 0; i < size; i++) {
    float newFrequency = (channelDataBuffer[i] - minFrequency) / (maxMinusMin);
		originalPositions[i].frequency = newFrequency < 0 ? 0 : (newFrequency > 1 ? 1 : newFrequency);
		//TODO: use some better method or more formal reperssentation of near zero
		//TODO: elsewhere toos
		// near 0 shows up as black, mby move this further in data setup
	}

	//TODO: refactor
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		for (auto p : originalPositions) {
			if (std::fabs(p.x - triangulatedPositions[i].x) < FLT_EPSILON && (std::fabs(p.y - triangulatedPositions[i].y) < FLT_EPSILON)) {
				triangulatedPositions[i].frequency = p.frequency;
			}
		}
	}
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
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

std::vector<GLfloat> ScalpCanvas::generateScalpTriangleArray() {
	std::vector<GLfloat> triangles;

	//TODO: refactor with single struct
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		//current position
		//TODO: can this be avoided?
		triangles.push_back(triangulatedPositions[i].x);
		triangles.push_back(triangulatedPositions[i].y);

		//frequency
		triangles.push_back(triangulatedPositions[i].frequency);


		//if (triangulatedPositions[i].frequency < 0.000001)
			//std::cout << "ZERO freq: " << triangulatedPositions[i].frequency << "\n";
	}

	return triangles;
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
	float maxMinusMinFreq = maxFrequency - minFrequency;

	float binY = maxMinusMinY / 4;
	float binFreq = maxMinusMinFreq / 4;

	float freqToDisplay = minFrequency;
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

//TODO: doesnt refresh on table change
void ScalpCanvas::paintGL() {
	using namespace chrono;

#ifndef NDEBUG
	logToFile("Painting started.");
#endif
	if (ready()) {
			//setup
			//makeCurrent();

			gl()->glUseProgram(channelProgram->getGLProgram());

			posBufferData.clear();

			//posBufferData = splitTriangles(splitTriangles(generateScalpTriangleArray()));

			posBufferData = generateScalpTriangleArray();

			std::vector<GLfloat> gradient = generateGradient();

			posBufferData.insert(std::end(posBufferData), std::begin(gradient), std::end(gradient));

			gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

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

			gl()->glFlush();

			// draw channels TODO: refactor - dont use points, add to main channel
			// TODO!!!:draw once and set it to be top of screen so it cant be drawn over
			//
			//
			//POINTS

			posBufferData.clear();

			//TODO: use positions, triangulatedPositions are inflated, repeated draws
			if (shouldDrawChannels) {
				for (int i = 0; i < originalPositions.size(); i++) {
							posBufferData.push_back(originalPositions[i].x);
							posBufferData.push_back(originalPositions[i].y);
				}
				gl()->glUseProgram(labelProgram->getGLProgram());
				gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

				gl()->glBufferData(GL_ARRAY_BUFFER, posBufferData.size() * sizeof(GLfloat), &posBufferData[0], GL_STATIC_DRAW);

				gl()->glEnableVertexAttribArray(0);
				gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
				gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
				gl()->glDrawArrays(GL_POINTS, 0, posBufferData.size());
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

		//doneCurrent();
	}

	if (errorMsg != "")
			renderErrorMsg();

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

}

bool ScalpCanvas::ready() {
	//TODO: think this through
		return shouldDraw && triangulatedPositions.size() > 0;
}

void ScalpCanvas::drawCircle(float cx, float cy, float r, int num_segments)
{
	float theta = 2 * 3.1415926 / float(num_segments);
	float c = cosf(theta);//precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = r;//we start at angle = 0 
	float y = 0;

	glBegin(GL_LINE_LOOP);
	for (int ii = 0; ii < num_segments; ii++)
	{
		glVertex2f(x + cx, y + cy);//output vertex 

								   //apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
	glEnd();
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