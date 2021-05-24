#ifndef SCALPCANVAS_H
#define SCALPCANVAS_H

#include "SignalProcessor/lrucache.h"
#include "openglinterface.h"
#include "GraphicsTools/colormap.h"

#ifdef __APPLE__
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl_gl.h>
#endif

#ifdef __GNUC__
#include "float.h"
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

/**
 * @brief This class implements a GUI control for rendering signal data and
 * events.
 *
 * Every time this control gets updated the whole surface gets repainted.
 *
 * This class also handles user keyboard and mouse input. Scrolling related
 * events are skipped and handled by the parent.
 */

//TODO: move to some utils class
class ElectrodePosition {
public:
	ElectrodePosition(GLfloat x, GLfloat y, GLfloat frequency) : x(x), y(y), frequency(frequency) { }
	ElectrodePosition(GLfloat x, GLfloat y) : x(x), y(y), frequency(0) { }

	bool operator == (const ElectrodePosition& position) const
	{
			return ((std::abs(position.x - x) < FLT_EPSILON && std::abs(position.y - y) <  FLT_EPSILON) ||
					(std::abs(position.x - y) <  FLT_EPSILON && std::abs(position.y - x) <  FLT_EPSILON));
	}

	bool operator != (const ElectrodePosition& position) const
	{
			return !((std::abs(position.x - x) <  FLT_EPSILON && std::abs(position.y - y) <  FLT_EPSILON) ||
					(std::abs(position.x - y) <  FLT_EPSILON && std::abs(position.y - x) <  FLT_EPSILON));
	}

	GLfloat x = 0;
	GLfloat y = 0;
	GLfloat frequency = 0;
};

/**
* @brief Distance coefficients of nearest points in tessalated triangle mesh.
*/
class PointCoefficient {
public:
  float coefficient;
  int toPoint;

  PointCoefficient(float coefficient, int toPoint) : coefficient(coefficient), toPoint(toPoint) {};
};

class ScalpCanvas : public QOpenGLWidget {
  Q_OBJECT
	
  std::unique_ptr<OpenGLProgram> labelProgram;
  std::unique_ptr<OpenGLProgram> channelProgram;
  std::vector<QString> labels;
  //TODO: should be pair of floats probably or custom class
	//TODO: mby move to scalpmap
  std::vector<ElectrodePosition> originalPositions;
  //TODO: replace with single struct
  std::vector<ElectrodePosition> triangulatedPositions;
  std::vector<GLfloat> splitTriangulatedPositions;
  std::vector<std::vector<PointCoefficient>> pointCoefficients;
	GLuint posBuffer;
	//TODO: possibly not needed
  std::vector<GLfloat> scalpMesh;
	QAction *setChannelDrawing;
  graphics::Colormap colormap;
  GLuint colormapTextureId;

  float minFrequency = 0;
  float maxFrequency = 0;

  const float gradientX = 0.9f;
  const float gradientBotY = -0.9f;
  const float gradientTopY = 0.7f;
	QString errorMsg;

	bool shouldDrawChannels = false;
	bool shouldDrawLabels = false;
	bool shouldDraw = false;

public:
  explicit ScalpCanvas(QWidget *parent = nullptr);
  ~ScalpCanvas() override;

  //TODO: refactor, right now labels and positions are instanced twice, here and in scalpMap
  void setChannelLabels(const std::vector<QString>& channelLabels);
  void setChannelPositions(const std::vector<QVector2D>& channelPositions);
	void setPositionFrequencies(const std::vector<float>& channelDataBuffer, const float& min, const float& max);
	void forbidDraw(QString errorString);
	void allowDraw();
	void clear();

protected:
	void cleanup();
  void initializeGL() override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void paintGL() override;
  void resizeGL(int w, int h) override;


private:
  //! Multiply by this to convert virtual position to sample position.
  float virtualRatio();
  //! Returns the sample position of the left screen edge.
  float leftEdgePosition();

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

  void renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor);

	std::vector<ElectrodePosition> generateTriangulatedPositions(const std::vector<ElectrodePosition>& channels);
  std::vector<GLfloat> generateScalpTriangleArray();
  std::vector<GLfloat> generateGradient();
  std::vector<GLfloat> splitTriangles(const std::vector<GLfloat>& triangles);
  void calculateFrequencies(std::vector<GLfloat>& points);
  void calculateDistanceCoefficients(const std::vector<GLfloat>& points);
	void renderErrorMsg();
  void renderGradientText();
  void renderPopupMenu(const QPoint& pos);
  GLuint setupColormapTexture(std::vector<float> colormap);
  void updateColormapTexture();
  void setupScalpMesh();

};

#endif // SCALPCANVAS_H
