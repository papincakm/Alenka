#ifndef SCALPCANVAS_H
#define SCALPCANVAS_H

#include "../SignalProcessor/lrucache.h"
#include "../openglinterface.h"
#include "../GraphicsTools/colormap.h"
#include "../GraphicsTools/graphicsrectangle.h"

#ifdef __APPLE__
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl_gl.h>
#endif

#ifdef __GNUC__
#include "float.h"
#endif

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
**/

class ElectrodePosition {
public:
	ElectrodePosition(GLfloat x, GLfloat y, GLfloat frequency) : x(x), y(y), voltage(frequency) { }
	ElectrodePosition(GLfloat x, GLfloat y) : x(x), y(y), voltage(0) { }

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
	GLfloat voltage = 0;
  GLuint indice = 0;
};

/**
* @brief Distance coefficients of nearest points in tessalated triangle mesh.
*/
class PointSpatialCoefficient {
public:
  float coefficient;
  int toPoint;

  PointSpatialCoefficient(float coefficient, int toPoint) : coefficient(coefficient), toPoint(toPoint) {};
};

class ScalpCanvas : public QOpenGLWidget {
  Q_OBJECT
  std::unique_ptr<OpenGLProgram> labelProgram;
  std::unique_ptr<OpenGLProgram> channelProgram;
  std::vector<QString> labels;
  std::vector<ElectrodePosition> originalPositions;
  std::vector<GLfloat> scalpMesh;
  std::vector<GLuint> indices;
  int uniqueIndiceCount = 0;
  std::vector<std::vector<PointSpatialCoefficient>> pointSpatialCoefficients;
  GLuint posBuffer;
  GLuint indexBuffer;
  graphics::Colormap colormap;
  GLuint colormapTextureId;
  std::unique_ptr<graphics::Gradient> gradient;

  QAction *setChannelDrawing;

  float minVoltage = 0;
  float maxVoltage = 0;


	QString errorMsg;

	bool shouldDrawChannels = false;
	bool shouldDrawLabels = false;
	bool dataReadyToDraw = false;
  bool glInitialized = false;
  bool printTiming = false;

  std::string lastGLMessage;
  int lastGLMessageCount = 0;

public:
  explicit ScalpCanvas(QWidget *parent = nullptr);
  ~ScalpCanvas() override;

  void setChannelLabels(const std::vector<QString>& channelLabels);
  void setChannelPositions(const std::vector<QVector2D>& channelPositions);
	void setPositionVoltages(const std::vector<float>& channelDataBuffer, const float& min, const float& max);
	void forbidDraw(QString errorString);
	void allowDraw();
	void clear();

protected:
	void cleanup();
  void initializeGL() override;
	void paintGL() override;
  void resizeGL(int w, int h) override;
  void mousePressEvent(QMouseEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;
  void mouseDoubleClickEvent(QMouseEvent* e) override;


private:
  void logLastGLMessage();

  bool ready();

  void renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor);

	std::vector<ElectrodePosition> generateTriangulatedGrid(const std::vector<ElectrodePosition>& channels);
  std::vector<GLfloat> generateScalpTriangleArray(const std::vector<ElectrodePosition>& triangulatedPositions);
  void splitTriangles(std::vector<ElectrodePosition>& triangulatedPositions, std::vector<GLuint>& indices);
  GLuint addMidEdgePoint(std::vector<ElectrodePosition>& splitTriangles,
    std::vector<GLuint>& splitIndices, ElectrodePosition candidate);
  void calculateVoltages(std::vector<GLfloat>& points);
  void calculateSpatialCoefficients(const std::vector<GLfloat>& points);
	void renderErrorMsg();
  void renderGradientText();
  GLuint setupColormapTexture(std::vector<float> colormap);
  void updateColormapTexture();
  void setupScalpMesh();
  void checkGLMessages();
  void genBuffers();
  void deleteBuffers();

  //menu
  void renderPopupMenu(const QPoint& pos);
  QMenu* setupExtremaMenu(QMenu* menu);
  QMenu* setupProjectionMenu(QMenu* menu);
};

#endif // SCALPCANVAS_H
