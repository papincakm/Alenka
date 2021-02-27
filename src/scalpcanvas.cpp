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
	updatePositionTriangles();
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

	//COMP D G
	triangleComplimentaryVertices.push_back(5);
	triangleComplimentaryVertices.push_back(14);
	triangleComplimentaryVertices.push_back(-1);

	//C B D
	triangulatedPositions.push_back(ElectrodePositionColored(-0.26021, 0.18427, cF, QVector3D(cF, bF, dF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(cF, bF, dF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(cF, bF, dF)));

	//COMP A E
	triangleComplimentaryVertices.push_back(0);
	triangleComplimentaryVertices.push_back(8);
	triangleComplimentaryVertices.push_back(-1);

	//B D E
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(bF, dF, eF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(bF, dF, eF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(bF, dF, eF)));

	//COMP C G F
	triangleComplimentaryVertices.push_back(2);
	triangleComplimentaryVertices.push_back(14);
	triangleComplimentaryVertices.push_back(11);

	//D E F
	triangulatedPositions.push_back(ElectrodePositionColored(-0.03607, 0.37819, dF, QVector3D(dF, eF, fF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(dF, eF, fF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.11316, 0.33157, fF, QVector3D(dF, eF, fF)));

	//COMP B H
	triangleComplimentaryVertices.push_back(1);
	triangleComplimentaryVertices.push_back(20);
	triangleComplimentaryVertices.push_back(-1);

	//A B G
	triangulatedPositions.push_back(ElectrodePositionColored(-0.25721, -0.07583, aF, QVector3D(aF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(aF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(aF, bF, gF)));

	//COMP C E I
	triangleComplimentaryVertices.push_back(2);
	triangleComplimentaryVertices.push_back(8);
	triangleComplimentaryVertices.push_back(26);

	//E B G
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(eF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.07736, 0.1205, bF, QVector3D(eF, bF, gF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(eF, bF, gF)));

	//COMP D A H
	triangleComplimentaryVertices.push_back(5);
	triangleComplimentaryVertices.push_back(0);
	triangleComplimentaryVertices.push_back(20);

	//G E H
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(gF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(gF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(gF, eF, hF)));

	//COMP B F J
	triangleComplimentaryVertices.push_back(1);
	triangleComplimentaryVertices.push_back(11);
	triangleComplimentaryVertices.push_back(27);

	//F E H
	triangulatedPositions.push_back(ElectrodePositionColored(0.11316, 0.33157, fF, QVector3D(fF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.05444, 0.13698, eF, QVector3D(fF, eF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(fF, eF, hF)));

	//COMP D H
	triangleComplimentaryVertices.push_back(5);
	triangleComplimentaryVertices.push_back(20);
	triangleComplimentaryVertices.push_back(-1);

	//A G I
	triangulatedPositions.push_back(ElectrodePositionColored(-0.25721, -0.07583, aF, QVector3D(aF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(aF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.11384, -0.33526, iF, QVector3D(aF, gF, iF)));

	//COMP B J
	triangleComplimentaryVertices.push_back(1);
	triangleComplimentaryVertices.push_back(27);
	triangleComplimentaryVertices.push_back(-1);

	//J G I
	triangulatedPositions.push_back(ElectrodePositionColored(0.03223, -0.20319, jF, QVector3D(jF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(jF, gF, iF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.11384, -0.33526, iF, QVector3D(jF, gF, iF)));

	//COMP A H
	triangleComplimentaryVertices.push_back(0);
	triangleComplimentaryVertices.push_back(20);
	triangleComplimentaryVertices.push_back(-1);

	//J G H
	triangulatedPositions.push_back(ElectrodePositionColored(0.03223, -0.20319, jF, QVector3D(jF, gF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(-0.06179, -0.07551, gF, QVector3D(jF, gF, hF)));
	triangulatedPositions.push_back(ElectrodePositionColored(0.072, -0.04267, hF, QVector3D(jF, gF, hF)));

	//COMP E I
	triangleComplimentaryVertices.push_back(8);
	triangleComplimentaryVertices.push_back(26);
	triangleComplimentaryVertices.push_back(-1);

    //generateComplimentaryVertices();

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
	//std::cout << "positions: " << positions.size() << "  channelBuffer: " << channelDataBuffer.size() << "freqs:\n";
	
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
	

	/*for (auto f : channelDataBuffer) {
		std::cout << " " << f;
	}

	std::cout << "\nConvertedFreqs:\n";

	for (auto f : positions) {
		std::cout << " " << f.frequency;
	}

	std::cout << "\n";*/

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

//TODO: refactor (dont like the ifs, make it better)
int ScalpCanvas::getComplimentaryVertex(const ElectrodePositionColored& first,
	const ElectrodePositionColored& second, const ElectrodePositionColored& third) {
	for (int i = 0; i < triangulatedPositions.size(); i += 3) {
		if (first == triangulatedPositions[i] && second == triangulatedPositions[i + 1] &&
			third != triangulatedPositions[i + 2]) {
			return i + 2;
		}

		if (first == triangulatedPositions[i + 1] && second == triangulatedPositions[i + 2] &&
			third != triangulatedPositions[i]) {
			return i;
		}

		if (first == triangulatedPositions[i] && second == triangulatedPositions[i + 2] &&
			third != triangulatedPositions[i + 1]) {
			return i + 1;
		}
	}
	// return default

	return -1;
}

//TODO: refactor
void ScalpCanvas::generateComplimentaryVertices() {
	assert(static_cast<int>(triangulatedPositions.size()) % 3 == 0);

	triangleComplimentaryVertices.clear();

	for (int i = 0; i < triangulatedPositions.size(); i += 3) {
		//first edge
		triangleComplimentaryVertices.push_back(getComplimentaryVertex(triangulatedPositions[i],
			triangulatedPositions[i + 1], triangulatedPositions[i + 2]));

		//second edge
		triangleComplimentaryVertices.push_back(getComplimentaryVertex(triangulatedPositions[i + 1],
			triangulatedPositions[i + 2], triangulatedPositions[i]));

		//third edge
		triangleComplimentaryVertices.push_back(getComplimentaryVertex(triangulatedPositions[i],
			triangulatedPositions[i + 2], triangulatedPositions[i + 1]));
	}
}

void ScalpCanvas::updatePositionTriangles() {
	updatePositionFrequencies();

	triangulatedPositions = generateScalpTriangleDrawPositions(positions);
	//triangleColors = generateScalpTriangleColors(triangleVertices);

	generateComplimentaryVertices();
	assert(static_cast<int>(triangleComplimentaryVertices.size()) == static_cast<int>(triangulatedPositions.size()));

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

	int barX = 0;
	int barY = 0;
	int barZ = 1;

	std::vector<GLfloat> vertices;
	std::vector<GLfloat> colors;
	int compCnt = 0;
	int triangleCnt = 0;

	//TODO: refactor with single struct
	for (int i = 0; i < triangulatedPositions.size(); i++) {
		//Barycentric  coords
		if (barZ == 1) {
			barX = 1;
			barY = 0;
			barZ = 0;
		}
		else if (barX == 1) {
			barX = 0;
			barY = 1;
			barZ = 0;
		}
		else if (barY == 1) {
			barX = 0;
			barY = 0;
			barZ = 1;
		}
		//frequency vec
		if (i % 3 == 0) {
			//setup triangle colors and vertices
			colors.clear();
			vertices.clear();

			triangleCnt++;

			for (int j = 0; j < 3; j++) {
				vertices.push_back(triangulatedPositions[i + j].x);
				vertices.push_back(triangulatedPositions[i + j].y);

				colors.push_back(triangulatedPositions[i + j].color.x());
				colors.push_back(triangulatedPositions[i + j].color.y());
				colors.push_back(triangulatedPositions[i + j].color.z());
			}

			for (int j = 0; j < 3; j++) {
				//i + j is valid vertex
				if (triangleComplimentaryVertices[i + j] >= 0) {
					vertices.push_back(triangulatedPositions[triangleComplimentaryVertices[i + j]].x);
					vertices.push_back(triangulatedPositions[triangleComplimentaryVertices[i + j]].y);

					colors.push_back(triangulatedPositions[triangleComplimentaryVertices[i + j]].color.x());
					colors.push_back(triangulatedPositions[triangleComplimentaryVertices[i + j]].color.y());
					colors.push_back(triangulatedPositions[triangleComplimentaryVertices[i + j]].color.z());
					compCnt++;
				}
				else {
					//vertex doesn't exist but opengl expects float data here
					//TODO: find out if its possible to avoid this
					vertices.push_back(-1);
					vertices.push_back(-1);

					colors.push_back(0);
					colors.push_back(0);
					colors.push_back(0);
				}
			}
		}
		//current position
		//TODO: can this be avoided?
		triangles.push_back(triangulatedPositions[i].x);
		triangles.push_back(triangulatedPositions[i].y);

		//triangle vertices
		assert(static_cast<int>(vertices.size()) == 12);
		triangles.insert(triangles.end(), vertices.begin(), vertices.end());

		//Colors
		//TODO: compute color from freq in vertex shader (deprecated?)
		assert(static_cast<int>(colors.size()) == 18);
		triangles.insert(triangles.end(), colors.begin(), colors.end());

		//Barycentric
		triangles.push_back(barX);
		triangles.push_back(barY);
		triangles.push_back(barZ);

		triangles.push_back(triangulatedPositions[i].frequency);
		if (triangulatedPositions[i].frequency < 0.000001)
			std::cout << "ZERO freq: " << triangulatedPositions[i].frequency << "\n";
	}

	std::cout << "complimentary: " << compCnt << "\n";
	std::cout << "triangles: " << triangleCnt << "\n";

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

		posBufferData.clear();

		//Create a 1D image - for this example it's just a red line
		//test
		/*const int stripeImageWidth = 32;
		GLubyte stripeImage[3 * stripeImageWidth];
		for (int j = 0; j < stripeImageWidth; j++) {
			stripeImage[3 * j] = j * 255 / 32; // use a gradient instead of a line
			stripeImage[3 * j + 1] = 255;
			stripeImage[3 * j + 2] = 255;
		}

		GLuint texID;
		gl()->glGenTextures(1, &texID);
		gl()->glBindTexture(GL_TEXTURE_1D, texID);
		gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		gl()->glTexImage1D(GL_TEXTURE_1D, 0, 3, stripeImageWidth, 0, GL_RGB, GL_UNSIGNED_BYTE, stripeImage);

		gl()->glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		gl()->glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl()->glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		gl()->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		gl()->glDisable(GL_TEXTURE_GEN_S);
		gl()->glDisable(GL_TEXTURE_2D);
		gl()->glEnable(GL_TEXTURE_1D);
		gl()->glBindTexture(GL_TEXTURE_1D, texID);


		const int grid_height = 100;
		const int grid_width = 100;

		GLfloat hist2D[grid_width][grid_height];

		for (int y = 0; y < grid_height; y++) {
			for (int x = 0; x < grid_width; x++) {
				hist2D[x][y] = x * y;
			}
		}



		gl()->glFlush();
		*/

		posBufferData = generateScalpTriangleArray();

		gl()->glBindBuffer(GL_ARRAY_BUFFER, posBuffer);

		gl()->glBufferData(GL_ARRAY_BUFFER, posBufferData.size() * sizeof(GLfloat), &posBufferData[0], GL_STATIC_DRAW);
		
		// 1st attribute buffer : vertices
		//current position
		gl()->glEnableVertexAttribArray(0);
		gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 36, (void*)0);

		//vertices
		int offset = 2;
		for (int i = 1; i < 7; i++) {
			gl()->glEnableVertexAttribArray(i);
			gl()->glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 36, (char*)(sizeof(GLfloat) * offset));
			offset += 2;
		}

		//colors
		for (int i = 7; i < 14; i++) {
			gl()->glEnableVertexAttribArray(i);
			gl()->glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 36, (char*)(sizeof(GLfloat) * offset));
			offset += 3;
		}

		gl()->glEnableVertexAttribArray(14);
		gl()->glVertexAttribPointer(14, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 36, (char*)(sizeof(GLfloat) * offset));


		gl()->glDrawArrays(GL_TRIANGLES, 0, triangulatedPositions.size());
		for (int i = 0; i < 15; i++) {
			gl()->glDisableVertexAttribArray(i);
		}

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