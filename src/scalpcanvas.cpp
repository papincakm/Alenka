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

ScalpCanvas::ScalpCanvas(QWidget *parent) : QOpenGLWidget(parent) {

}

ScalpCanvas::~ScalpCanvas() {
  makeCurrent();

  // Release these three objects here explicitly to make sure the right GL
  // context is bound by makeCurrent().
  //signalProgram.reset();

  logLastGLMessage();
  OPENGL_INTERFACE->checkGLErrors();
  doneCurrent();
}

void ScalpCanvas::setChannelPositions(const std::vector<QVector2D>& positions) {
	channelPositions = positions;
}

void ScalpCanvas::initializeGL() {
	logToFile("Initializing OpenGL in ScalpCanvas.");

	OPENGL_INTERFACE = make_unique<OpenGLInterface>();
	OPENGL_INTERFACE->initializeOpenGLInterface();

	/*QFile signalVertFile(":/signal.vert");
	signalVertFile.open(QIODevice::ReadOnly);
	string signalVert = signalVertFile.readAll().toStdString();

	QFile colorFragFile(":/color.frag");
	colorFragFile.open(QIODevice::ReadOnly);
	string colorFrag = colorFragFile.readAll().toStdString();

	signalProgram = make_unique<OpenGLProgram>(signalVert, colorFrag);

	QFile circleFragFile(":/circle.frag");
	circleFragFile.open(QIODevice::ReadOnly);
	string circleFrag = circleFragFile.readAll().toStdString();

	channelProgram = make_unique<OpenGLProgram>(circleFrag);
	*/
	createContext();
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
}

void ScalpCanvas::paintGL() {
	using namespace chrono;

	if (paintingDisabled)
		return;

#ifndef NDEBUG
	logToFile("Painting started.");
#endif
	if (ready()) {
		glColor3f(1, 0, 0);

		for (auto v : channelPositions) {
			drawCircle(v.x(), v.y(), 0.01, 30);
		}
	}
}

float ScalpCanvas::virtualRatio() {
	return 0;
}

float ScalpCanvas::leftEdgePosition() {
  return OpenDataFile::infoTable.getPosition() -
         OpenDataFile::infoTable.getPositionIndicator() * width() *
             virtualRatio();
}

void ScalpCanvas::createContext() {

}


void ScalpCanvas::logLastGLMessage() {

}

bool ScalpCanvas::ready() {
	return true;
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