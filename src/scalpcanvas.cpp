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
		positions.push_back(ElectrodePosition(-1 * v.y(), v.x()));
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

	float aF = 0.2f;
	float bF = 0.9f;
	float cF = 0.6f;
	float dF = 0.3f;
	float eF = 0.2f;
	float fF = 0.2f;
	float gF = 0.3f;
	float hF = 0.9f;
	float iF = 0.2f;
	float jF = 0.7f;

	/*//initialize data
	//A B C
	triangulatedPositions.push_back(ElectrodePosition(-0.25721, -0.07583, aF));
	triangulatedPositions.push_back(ElectrodePosition(-0.07736, 0.1205, bF));
	triangulatedPositions.push_back(ElectrodePosition(-0.26021, 0.18427, cF));

	//C B D
	triangulatedPositions.push_back(ElectrodePosition(-0.26021, 0.18427, cF));
	triangulatedPositions.push_back(ElectrodePosition(-0.07736, 0.1205, bF));
	triangulatedPositions.push_back(ElectrodePosition(-0.03607, 0.37819, dF));

	//B D E
	triangulatedPositions.push_back(ElectrodePosition(-0.07736, 0.1205, bF));
	triangulatedPositions.push_back(ElectrodePosition(-0.03607, 0.37819, dF));
	triangulatedPositions.push_back(ElectrodePosition(0.05444, 0.13698, eF));

	//D E F
	triangulatedPositions.push_back(ElectrodePosition(-0.03607, 0.37819, dF));
	triangulatedPositions.push_back(ElectrodePosition(0.05444, 0.13698, eF));
	triangulatedPositions.push_back(ElectrodePosition(0.11316, 0.33157, fF));

	//A B G
	triangulatedPositions.push_back(ElectrodePosition(-0.25721, -0.07583, aF));
	triangulatedPositions.push_back(ElectrodePosition(-0.07736, 0.1205, bF));
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));

	//E B G
	triangulatedPositions.push_back(ElectrodePosition(0.05444, 0.13698, eF));
	triangulatedPositions.push_back(ElectrodePosition(-0.07736, 0.1205, bF));
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));

	//G E H
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));
	triangulatedPositions.push_back(ElectrodePosition(0.05444, 0.13698, eF));
	triangulatedPositions.push_back(ElectrodePosition(0.072, -0.04267, hF));

	//F E H
	triangulatedPositions.push_back(ElectrodePosition(0.11316, 0.33157, fF));
	triangulatedPositions.push_back(ElectrodePosition(0.05444, 0.13698, eF));
	triangulatedPositions.push_back(ElectrodePosition(0.072, -0.04267, hF));
	
	//A G I
	triangulatedPositions.push_back(ElectrodePosition(-0.25721, -0.07583, aF));
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));
	triangulatedPositions.push_back(ElectrodePosition(-0.11384, -0.33526, iF));
	
	//J G I
	triangulatedPositions.push_back(ElectrodePosition(0.03223, -0.20319, jF));
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));
	triangulatedPositions.push_back(ElectrodePosition(-0.11384, -0.33526, iF));

	//J G H
	triangulatedPositions.push_back(ElectrodePosition(0.03223, -0.20319, jF));
	triangulatedPositions.push_back(ElectrodePosition(-0.06179, -0.07551, gF));
	triangulatedPositions.push_back(ElectrodePosition(0.072, -0.04267, hF));
	*/

	//A B C
	triangulatedPositions.push_back(ElectrodePositionColored(-0.25721, -0.07583, aF, QVector3D(aF, bF, cF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(aF, bF, cF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.26021, 0.18427, cF, QVector3D(aF, bF, cF)));

	//C B D
	triangulatedPositions.push_back(ElectrodePositionColored(-0.26021, 0.18427, cF, QVector3D(cF, bF, dF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(cF, bF, dF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(cF, bF, dF)));

	//B D E
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(bF, dF, eF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(bF, dF, eF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(bF, dF, eF)));

	//D E F
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(dF, eF, fF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(dF, eF, fF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.11316, 0.33157, fF, QVector3D(dF, eF, fF)));

	//A B G
	triangulatedPositions.push_back(ElectrodePositionColored(-0.25721, -0.07583, aF, QVector3D(aF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(aF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(aF, bF, gF)));

	//E B G
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(eF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(eF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(eF, bF, gF)));

	//G E H
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(gF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(gF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(gF, eF, hF)));

	//F E H
	triangulatedPositions.push_back(ElectrodePositionColored(0.11316, 0.33157, fF, QVector3D(fF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(fF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(fF, eF, hF)));

	//A G I
	triangulatedPositions.push_back(ElectrodePositionColored(-0.25721, -0.07583, aF, QVector3D(aF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(aF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.11384, -0.33526, iF, QVector3D(aF, gF, iF)));

	//J G I
	triangulatedPositions.push_back(ElectrodePositionColored(0.03223, -0.20319, jF, QVector3D(jF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(jF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.11384, -0.33526, iF, QVector3D(jF, gF, iF)));

	//J G H
	triangulatedPositions.push_back(ElectrodePositionColored(0.03223, -0.20319, jF, QVector3D(jF, gF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(jF, gF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(jF, gF, hF)));

	posBufferData = generateScalpTriangleArray();

	gl()->glGenBuffers(1, &posBuffer);

	createContext();
}



QVector3D getFreqColor(const float& oFrequency) {
	const float firstT = 0.25f;
	const float secondT = 0.5f;
	const float thirdT = 0.75f;

	float red = 0;
	float green = 0;
	float blue = 0;

	if (oFrequency < firstT) {
		red = 0;
		green = oFrequency * 4;
		blue = 1;
	}
	else if (oFrequency < secondT) {
		red = 0;
		green = 1;
		blue = 1 - ((oFrequency - firstT) * 4);
	}
	else if (oFrequency < thirdT) {
		red = ((oFrequency - secondT) * 4);
		green = 1;
		blue = 0;
	}
	else {
		red = 1;
		green = 1 - ((oFrequency - thirdT) * 4);
		blue = 0;
	}

	return QVector3D(red, green, blue);
}

void ScalpCanvas::updatePositionFrequencies() {

	srand(time(NULL));

	for (int i = 0; i < positions.size(); i++) {
		positions[i].frequency = ((rand() % 100 + 1) / 100.0f);
	}

	//TODO: refactor
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		for (auto p : positions) {
			if (p.x - triangulatedPositions[i].x < 0.00001 && (p.y - triangulatedPositions[i].y < 0.00001)) {
				triangulatedPositions[i].color = getFreqColor(p.frequency);
			}
		}
	}

	update();
}

void ScalpCanvas::updatePositionFrequencies(const std::vector<float>& channelDataBuffer) {
	//TODO: theres less positions thant channelDataBuffer
	assert(static_cast<int>(positions.size()) < static_cast<int>(channelDataBuffer.size()));
	std::cout << "positions: " << positions.size() << "  channelBuffer: " << channelDataBuffer.size() << "freqs:\n";
	
	float max = std::numeric_limits<int>::min();
	float min = std::numeric_limits<int>::max();
	for (int i = 0; i < positions.size(); i++) {
		if (channelDataBuffer[i] > max)
			max = channelDataBuffer[i];
		if (channelDataBuffer[i] < min)
			min = channelDataBuffer[i];
	}

	max -= min;

	for (int i = 0; i < positions.size(); i++) {
		positions[i].frequency = (channelDataBuffer[i] - min) / max;
	}
	

	for (auto f : channelDataBuffer) {
		std::cout << " " << f;
	}

	std::cout << "\nConvertedFreqs:\n";

	for (auto f : positions) {
		std::cout << " " << f.frequency;
	}

	std::cout << "\n";

	//TODO: refactor
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		for (auto p : positions) {
			if (p.x - triangulatedPositions[i].x < 0.00001 && (p.y - triangulatedPositions[i].y < 0.00001)) {
				triangulatedPositions[i].color = getFreqColor(p.frequency);
				triangulatedPositions[i].frequency = p.frequency;
			}
		}
	}

	update();
}

void ScalpCanvas::updatePositions() {
	updatePositionFrequencies();

	triangulatedPositions = generateScalpTriangleDrawPositions(positions);
	//triangleColors = generateScalpTriangleColors(triangleVertices);

	//TODO: refactor
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		for (auto p : positions) {
			if (p.x - triangulatedPositions[i].x < 0.00001 && (p.y - triangulatedPositions[i].y < 0.00001)) {
				triangulatedPositions[i].color = getFreqColor(p.frequency);
			}
		}
	}

	//update();
}

void ScalpCanvas::resizeGL(int /*w*/, int /*h*/) {
  // checkGLMessages();
}

//TODO: testing, refactor in future, use less data containers
std::vector<ElectrodePositionColored> ScalpCanvas::generateScalpTriangleDrawPositions(std::vector<ElectrodePosition> channels) {
	std::vector<double> coords;
	std::vector<ElectrodePositionColored> triangles;
	
	for (auto ch : channels) {
		coords.push_back(ch.x);
		coords.push_back(ch.y);
	}

	delaunator::Delaunator d(coords);
	
	for (std::size_t i = 0; i < d.triangles.size(); i+=3) {
		triangles.push_back(ElectrodePositionColored(d.coords[2 * d.triangles[i]],
											  d.coords[2 * d.triangles[i] + 1]));
		triangles.push_back(ElectrodePositionColored(d.coords[2 * d.triangles[i + 1]],
											  d.coords[2 * d.triangles[i + 1] + 1]));
		triangles.push_back(ElectrodePositionColored(d.coords[2 * d.triangles[i + 2]],
											  d.coords[2 * d.triangles[i + 2] + 1]));
	}

	return triangles;
}

std::vector<QVector3D> ScalpCanvas::generateScalpTriangleColors(std::vector<ElectrodePosition> pos) {
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
	float barX = 0;
	float barY = 0;
	float barZ = 1;
	QVector2D posVecA;
	QVector2D posVecB;
	QVector2D posVecC;

	QVector3D freqVec;
	QVector3D vertAColor;
	QVector3D vertBColor;
	QVector3D vertCColor;

	//TODO: refactor with single struct
	for (int i = 0; i < triangulatedPositions.size(); i++) {

		//frequency vec
		if (i % 3 == 0) {
			freqVec = QVector3D(triangulatedPositions[i].frequency, triangulatedPositions[i + 1].frequency,
							    triangulatedPositions[i + 2].frequency);
			vertAColor = QVector3D(triangulatedPositions[i].color.x(), triangulatedPositions[i].color.y(),
								   triangulatedPositions[i].color.z());

			vertBColor = QVector3D(triangulatedPositions[i + 1].color.x(), triangulatedPositions[i + 1].color.y(),
								   triangulatedPositions[i + 1].color.z());

			vertCColor = QVector3D(triangulatedPositions[i + 2].color.x(), triangulatedPositions[i + 2].color.y(),
								   triangulatedPositions[i + 2].color.z());

			posVecA = QVector2D(triangulatedPositions[i].x, triangulatedPositions[i].y);
			posVecB = QVector2D(triangulatedPositions[i + 1].x, triangulatedPositions[i + 1].y);
			posVecC = QVector2D(triangulatedPositions[i + 2].x, triangulatedPositions[i + 2].y);
		}

		triangles.push_back(triangulatedPositions[i].x);
		triangles.push_back(triangulatedPositions[i].y);
		triangles.push_back(posVecA.x());
		triangles.push_back(posVecA.y());
		triangles.push_back(posVecB.x());
		triangles.push_back(posVecB.y());
		triangles.push_back(posVecC.x());
		triangles.push_back(posVecC.y());

		//Colors
		//TODO: use array or better(?), compute color from freq in vertex shader
		triangles.push_back(vertAColor.x());
		triangles.push_back(vertAColor.y());
		triangles.push_back(vertAColor.z());

		triangles.push_back(vertBColor.x());
		triangles.push_back(vertBColor.y());
		triangles.push_back(vertBColor.z());

		triangles.push_back(vertCColor.x());
		triangles.push_back(vertCColor.y());
		triangles.push_back(vertCColor.z());
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
		gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (void*)0);

		gl()->glEnableVertexAttribArray(1);
		gl()->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*)(sizeof(GLfloat) * 2));

		gl()->glEnableVertexAttribArray(2);
		gl()->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*)(sizeof(GLfloat) * 4));

		gl()->glEnableVertexAttribArray(3);
		gl()->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*)(sizeof(GLfloat) * 6));

		gl()->glEnableVertexAttribArray(4);
		gl()->glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*) (sizeof(GLfloat) * 8));

		gl()->glEnableVertexAttribArray(5);
		gl()->glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*)(sizeof(GLfloat) * 11));

		gl()->glEnableVertexAttribArray(6);
		gl()->glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 17, (char*)(sizeof(GLfloat) * 14));

		gl()->glDrawArrays(GL_TRIANGLES, 0, triangulatedPositions.size());
		gl()->glDisableVertexAttribArray(0);
		gl()->glDisableVertexAttribArray(1);
		gl()->glDisableVertexAttribArray(2);
		gl()->glDisableVertexAttribArray(3);
		gl()->glDisableVertexAttribArray(4);
		gl()->glDisableVertexAttribArray(5);
		gl()->glDisableVertexAttribArray(6);


		// draw channels TODO: refactor - dont use points
		/*gl()->glUseProgram(labelProgram->getGLProgram());

		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

		gl()->glBufferData(GL_ARRAY_BUFFER, triangulatedPositions.size() * sizeof(ElectrodePosition), &triangulatedPositions[0], GL_STATIC_DRAW);

		gl()->glEnableVertexAttribArray(0);
		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
		gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gl()->glDrawArrays(GL_POINTS, 0, triangulatedPositions.size());
		gl()->glDisableVertexAttribArray(0);

		gl()->glFlush();

		for (int i = 0; i < positions.size(); i++) {
			glColor3f(0.6, 0.6, 0.6);
			renderText(positions[i].x, -1 * positions[i].y - 0.02, labels[i], labelFont);
		}*/
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