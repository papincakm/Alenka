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

  channelProgram.reset();
  labelProgram.reset();

  logLastGLMessage();
  OPENGL_INTERFACE->checkGLErrors();
  doneCurrent();
}

void ScalpCanvas::setChannelPositions(const std::vector<QVector2D>& channelPositions) {
	positions.clear();
	
	for (auto v : channelPositions) {
		positions.push_back(QVector2D(-1 * v.y(), v.x()));
	}
	
	//TODO: refactor me
	updatePositions();
}
void ScalpCanvas::setChannelLabels(const std::vector<QString>& channelLabels) {
	labels = channelLabels;
}

void ScalpCanvas::addTrack(const QString& label, const QVector2D& position) {
	tracks.push_back(CanvasTrack(label, position));
}

void ScalpCanvas::resetTracks() {
	tracks.clear();
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
	logToFile("Initializing OpenGL in ScalpCanvas.");

	OPENGL_INTERFACE = make_unique<OpenGLInterface>();
	OPENGL_INTERFACE->initializeOpenGLInterface();

	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SPRITE);
	
	//circle version
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

	//initialize data
	trianglePositions.push_back(QVector2D(0.5f, 0.5f));
	trianglePositions.push_back(QVector2D(0.0f, 0.0f));
	trianglePositions.push_back(QVector2D(0.5f, -0.5f));
	triangleColors.push_back(QVector3D(1.0f, 0.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 1.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 0.0f, 1.0f));

	trianglePositions.push_back(QVector2D(-0.45f, -0.45f));
	trianglePositions.push_back(QVector2D(-0.45f, 0.0f));
	trianglePositions.push_back(QVector2D(-0.25f, -0.25f));
	triangleColors.push_back(QVector3D(1.0f, 0.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 1.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 0.0f, 1.0f));

	trianglePositions.push_back(QVector2D(-0.45f, -0.45f));
	trianglePositions.push_back(QVector2D(-0.45f, 0.0f));
	trianglePositions.push_back(QVector2D(-0.25f, -0.25f));
	triangleColors.push_back(QVector3D(1.0f, 0.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 1.0f, 0.0f));
	triangleColors.push_back(QVector3D(0.0f, 0.0f, 1.0f));

	posBufferData = generateScalpTriangleArray();

	gl()->glGenBuffers(1, &posBuffer);

	createContext();
}

void ScalpCanvas::updatePositions() {
	trianglePositions = generateScalpTrianglePositions(positions);

	triangleColors = generateScalpTriangleColors(trianglePositions);
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
}

//TODO: move to separate file
std::vector<Edge> generateAllEdges(std::vector<QVector2D> vertices) {
	std::vector<Edge> edges;

	for (int i = 0; i < vertices.size(); i++) {
		auto temp = vertices;
		temp.erase(temp.begin() + i);

		for (auto v : temp) {
			Edge newEdge;
			newEdge.from = vertices[i];
			newEdge.to = v;
			newEdge.length = vertices[i].distanceToPoint(v);

			edges.push_back(newEdge);
		}
	}

	return edges;
}

//https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/
// Given three colinear points p, q, r, the function checks if 
// point q lies on line segment 'pr' 
bool onSegment(QVector2D p, QVector2D q, QVector2D r)
{
	if (q.x() <= std::max(p.x(), r.x()) && q.x() >= std::min(p.x(), r.x()) &&
		q.y() <= std::max(p.y(), r.y()) && q.y() >= std::min(p.y(), r.y()))
		return true;

	return false;
}

// To find orientation of ordered triplet (p, q, r). 
// The function returns following values 
// 0 --> p, q and r are colinear 
// 1 --> Clockwise 
// 2 --> Counterclockwise 
int orientation(QVector2D p, QVector2D q, QVector2D r)
{
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/ 
	// for details of below formula. 
	int val = (q.y() - p.y()) * (r.x() - q.x()) -
		(q.x() - p.x()) * (r.y() - q.y());

	if (val == 0) return 0;  // colinear 

	return (val > 0) ? 1 : 2; // clock or counterclock wise 
}


// The main function that returns true if line segment 'p1q1' 
// and 'p2q2' intersect. 
bool doIntersect(QVector2D p1, QVector2D q1, QVector2D p2, QVector2D q2)
{
	// Find the four orientations needed for general and 
	// special cases 
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case 
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases 
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1 
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are colinear and q2 lies on segment p1q1 
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2 
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2 
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases 
}

std::vector<QVector2D> ScalpCanvas::generateScalpTrianglePositionsBetter(std::vector<QVector2D> channels) {
	std::vector<QVector2D> triangles;

	std::vector<Edge> edges = generateAllEdges(channels);

	std::sort(edges.begin(), edges.end(), greater<Edge>());

	//remove crossing edges

	return triangles;
}

std::vector<QVector2D> ScalpCanvas::generateScalpTrianglePositions(std::vector<QVector2D> channels) {
	std::vector<QVector2D> triangles;

	//TODO: refactor this
	for (int i = 0; i < channels.size(); i++) {
		auto temp = channels;
		temp.erase(temp.begin() + i);

		QVector2D point1;
		QVector2D point2;

		float nearestPointDist1 = 999999;
		float nearestPointDist2 = 999999;

		//TODO: can sometimes pick already used combinaion of three points
		for (auto n : temp) {
			float nDist = channels[i].distanceToPoint(n);

			if (nDist < nearestPointDist1) {
				point1 = n;
				nearestPointDist1 = channels[i].distanceToPoint(n);
				continue;
			}

			if (nDist < nearestPointDist2) {
				point2 = n;
				nearestPointDist2 = channels[i].distanceToPoint(n);
			}
		}

		triangles.push_back(QVector2D(channels[i]));
		triangles.push_back(point1);
		triangles.push_back(point2);
	}

	return triangles;
}

std::vector<QVector3D> ScalpCanvas::generateScalpTriangleColors(std::vector<QVector2D> pos) {
	std::vector<QVector3D> colors;

	for (int i = 0; i < pos.size(); i++) {
		if (i % 3 == 1)
			colors.push_back(QVector3D(1.0f, 0.0f, 0.0f));
		if (i % 3 == 2)
			colors.push_back(QVector3D(0.0f, 1.0f, 0.0f));
		if (i % 3 == 0)
			colors.push_back(QVector3D(0.0f, 0.0f, 1.0f));
	}

	return colors;
}

std::vector<GLfloat> ScalpCanvas::generateScalpTriangleArray() {
	std::vector<GLfloat> triangles;

	//TODO: refactor with single struct
	for (int i = 0; i < trianglePositions.size(); i++) {
		triangles.push_back(trianglePositions[i].x());
		triangles.push_back(trianglePositions[i].y());

		triangles.push_back(triangleColors[i].x());
		triangles.push_back(triangleColors[i].y());
		triangles.push_back(triangleColors[i].z());
	}

	return triangles;
}

//TODO: doesnt refresh on table change
void ScalpCanvas::paintGL() {
	using namespace chrono;

	if (paintingDisabled)
		return;
	
	QString test = QString("TEST");
	QFont labelFont = QFont("Times", 8, QFont::Bold);

#ifndef NDEBUG
	logToFile("Painting started.");
#endif
	if (ready()) {
		//setup
		gl()->glUseProgram(channelProgram->getGLProgram());

		//posBufferData.clear();
		posBufferData = generateScalpTriangleArray();

		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

		gl()->glBufferData(GL_ARRAY_BUFFER, posBufferData.size() * sizeof(GLfloat), &posBufferData[0], GL_STATIC_DRAW);

		
		// 1st attribute buffer : vertices
		gl()->glEnableVertexAttribArray(0);
		gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*)0);

		gl()->glEnableVertexAttribArray(1);
		gl()->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (char*) (sizeof(GLfloat) * 2));

		gl()->glDrawArrays(GL_TRIANGLES, 0, trianglePositions.size());
		gl()->glDisableVertexAttribArray(0);
		gl()->glDisableVertexAttribArray(1);


		// draw channels TODO: refactor - dont use points
		gl()->glUseProgram(labelProgram->getGLProgram());

		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

		gl()->glBufferData(GL_ARRAY_BUFFER, trianglePositions.size() * sizeof(QVector2D), &trianglePositions[0], GL_STATIC_DRAW);

		gl()->glEnableVertexAttribArray(0);
		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
		gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gl()->glDrawArrays(GL_POINTS, 0, trianglePositions.size());
		gl()->glDisableVertexAttribArray(0);

		gl()->glFlush();

		for (int i = 0; i < positions.size(); i++) {
			glColor3f(0.6, 0.6, 0.6);
			renderText(positions[i].x(), -1 * positions[i].y() - 0.02, labels[i], labelFont);
		}
	}

	gl()->glFinish();
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

void ScalpCanvas::renderText(float x, float y, const QString& str, const QFont& font) {
	int realX = width() / 2 + (width() / 2) * x;
	int realY = height() / 2 + (height() / 2) * y;

	GLdouble glColor[4];
	glGetDoublev(GL_CURRENT_COLOR, glColor);
	QColor fontColor = QColor(glColor[0] * 255, glColor[1] * 255, glColor[2] * 255, glColor[3] * 255);

	//QColor fontColor = QColor(0.6, 0.6, 0.6);

	// Render text
	QPainter painter(this);
	painter.setPen(fontColor);
	painter.setFont(font);
	painter.drawText(realX, realY, str);
	painter.end();
}