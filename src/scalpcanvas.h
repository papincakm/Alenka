#ifndef SCALPCANVAS_H
#define SCALPCANVAS_H

#include "SignalProcessor/lrucache.h"
#include "openglinterface.h"

#ifdef __APPLE__
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl_gl.h>
#endif

#include <QOpenGLWidget>
#include <QPainter>
#include <QString>
#include <QFont>
#include <QVector2D>
#include <cassert>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <QVector3D>

namespace AlenkaSignal {
class OpenCLContext;
}
class OpenDataFile;
class OpenGLProgram;

struct CanvasTrack {
	CanvasTrack(const QString& label, const QVector2D& positions) : label(label), position(position) {};

	QString label;
	QVector2D position;
};

struct Edge {
	QVector2D from;
	QVector2D to;
	float length;

	bool operator > (const Edge& edge) const
	{
		return (length > edge.length);
	}
};

/**
 * @brief This class implements a GUI control for rendering signal data and
 * events.
 *
 * Every time this control gets updated the whole surface gets repainted.
 *
 * This class also handles user keyboard and mouse input. Scrolling related
 * events are skipped and handled by the parent.
 */

class ElectrodePositionColored {
public:
	ElectrodePositionColored() : color(QVector3D(0, 0, 0)) {};
	ElectrodePositionColored(GLfloat x, GLfloat y, GLfloat frequency, QVector3D frequencyVec) : x(x), y(y),
		color(getFreqColor(frequency)), frequencyVec(frequencyVec), frequency(frequency) { }
	ElectrodePositionColored(GLfloat x, GLfloat y) : x(x), y(y), color(getFreqColor(0)) { }

	//TODO: change float compare values to smthng more standard
	bool operator == (const ElectrodePositionColored& position) const
	{
		return ((position.x - x < 0.00001 && position.y - y < 0.00001) || (position.x - y < 0.00001 && position.y - x < 0.00001));
	}

	bool operator != (const ElectrodePositionColored& position) const
	{
		return !((position.x - x < 0.00001 && position.y - y < 0.00001) || (position.x - y < 0.00001 && position.y - x < 0.00001));
	}

	GLfloat x = 0;
	GLfloat y = 0;
	QVector3D frequencyVec;
	GLfloat frequency = 0;
	QVector3D color;

private:

	QVector3D getFreqColor(const float& oFrequency) {
		const float firstT = 0.25;
		const float secondT = 0.5;
		const float thirdT = 0.75;

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
};

class ElectrodePosition {
public:
	ElectrodePosition(GLfloat x, GLfloat y, GLfloat frequency) : x(x), y(y), frequency(frequency) { }
	ElectrodePosition(GLfloat x, GLfloat y) : x(x), y(y), frequency(0) { }

	GLfloat x = 0;
	GLfloat y = 0;
	GLfloat frequency = 0;
};

class ScalpCanvas : public QOpenGLWidget {
  Q_OBJECT
	
  std::unique_ptr<OpenGLProgram> labelProgram;
  std::unique_ptr<OpenGLProgram> channelProgram;
  bool paintingDisabled = false;
  std::vector<CanvasTrack> tracks;
  std::vector<QString> labels;
  //TODO: should be pair of floats probably or custom class
  std::vector<ElectrodePosition> positions;
  GLuint posBuffer;
  //TODO: replace with single struct
  std::vector<ElectrodePositionColored> triangulatedPositions;
  std::vector<QVector3D> triangleColors;
  //TODO: for testing
  std::vector<float> triangleFrequencies;
  std::map<QVector2D, GLfloat> posFrequencies;
  std::vector<GLfloat> posBufferData;

  float minFrequency = 0;
  float maxFrequency = 0;

  const float gradientX = 0.9f;
  const float gradientBotY = -0.9f;
  const float gradientTopY = 0.7f;

public:
  explicit ScalpCanvas(QWidget *parent = nullptr);
  ~ScalpCanvas() override;

  //TODO: refactor, right now labels and positions are instanced twice, here and in scalpMap
  void setChannelLabels(const std::vector<QString>& channelLabels);
  void setChannelPositions(const std::vector<QVector2D>& channelPositions);
  void addTrack(const QString& label, const QVector2D& position);
  void resetTracks();
  void updatePositionTriangles();
  void updatePositionFrequencies(const std::vector<float>& channelDataBuffer);
  void updatePositionFrequencies();

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
  //! Multiply by this to convert virtual position to sample position.
  float virtualRatio();
  //! Returns the sample position of the left screen edge.
  float leftEdgePosition();

  void createContext();
  void logLastGLMessage();

  /**
   * @brief Tests whether SignalProcessor is ready to return blocks.
   *
   * This method is used to skip some code that would break if no file is
   * open and/or the current montage is empty.
   */
  bool ready();

  //TODO:: move to approp file
  //source: http://slabode.exofire.net/circle_draw.shtml
  void drawCircle(float cx, float cy, float r, int num_segments);

  void renderText(float x, float y, const QString &str, const QFont &font);

  std::vector<ElectrodePositionColored> generateScalpTriangleDrawPositions(std::vector<ElectrodePosition> channels);
  std::vector<QVector3D> generateScalpTriangleColors(std::vector<ElectrodePosition> channels);
  std::vector<GLfloat> generateScalpTriangleArray();
  std::vector<GLfloat> ScalpCanvas::generateGradient();
  void renderGradientText();
};

#endif // SCALPCANVAS_H
