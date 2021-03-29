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
	makeCurrent();

	// Release these three objects here explicitly to make sure the right GL
	// context is bound by makeCurrent().

	gl()->glDeleteBuffers(1, &posBuffer);

	channelProgram.reset();

	logLastGLMessage();

	OPENGL_INTERFACE->checkGLErrors();
	doneCurrent();
}

void TfVisualizer::cleanup() {
		makeCurrent();

		gl()->glDeleteBuffers(1, &posBuffer);

		channelProgram.reset();

		logLastGLMessage();
		OPENGL_INTERFACE->checkGLErrors();

		doneCurrent();
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

	gl()->glGenBuffers(1, &posBuffer);
	gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
	gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);

	createContext();
}

void TfVisualizer::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
}


void TfVisualizer::setDataToDraw(const std::vector<std::vector<QVector3D>>& grid) {
		posBufferData = triangulateGrid(grid);
}

void TfVisualizer::paintGL() {
	using namespace chrono;

	if (paintingDisabled)
		return;

#ifndef NDEBUG
	logToFile("Painting started.");
#endif
	if (ready()) {
			//gradient
			//posBufferData.insert(std::end(posBufferData), std::begin(gradient), std::end(gradient));

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
	}
}


bool TfVisualizer::ready() {
		return posBufferData.size();
}


void TfVisualizer::createContext() {

}


void TfVisualizer::logLastGLMessage() {

}

//TODO: dont call this when only freq gets changed
std::vector<GLfloat> TfVisualizer::triangulateGrid(const std::vector<std::vector<QVector3D>>& grid) {
		std::vector<GLfloat> triangles;

		for (int i = 0; i < grid.size() - 1; i++) {
				for (int j = 0; j < grid[i].size() - 1; j++) {
						//left triangle
						triangles.push_back(grid[i][j].x());
						triangles.push_back(grid[i][j].y());

						triangles.push_back(grid[i + 1][j].x());
						triangles.push_back(grid[i + 1][j].y());

						triangles.push_back(grid[i + 1][j + 1].x());
						triangles.push_back(grid[i + 1][j + 1].y());

						//right triangle
						triangles.push_back(grid[i][j].x());
						triangles.push_back(grid[i][j].y());

						triangles.push_back(grid[i][j + 1].x());
						triangles.push_back(grid[i][j + 1].y());

						triangles.push_back(grid[i + 1][j + 1].x());
						triangles.push_back(grid[i + 1][j + 1].y());
				}
		}

		return triangles;
}