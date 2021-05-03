#ifndef TFVISUALIZER_H
#define TFVISUALIZER_H

#include "openglinterface.h"
#include "GraphicsTools/colormap.h"
#include "GraphicsTools/graphicsrectangle.h"

#ifdef __APPLE__
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl_gl.h>
#endif

#include <QOpenGLWidget>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <QPainter>
#include <iostream>

namespace AlenkaSignal {
class OpenCLContext;
}
class OpenDataFile;
class OpenGLProgram;

struct GraphicsRectangle {
  GraphicsRectangle() {};
  GraphicsRectangle(float bX, float bY, float tX, float tY) : botX(bX), botY(bY), topX(tX), topY(tY) {};

  float botX = 0.0f;
  float botY = 0.0f;
  float topX = 0.0f;
  float topY = 0.0f;
};

struct GraphicsNumberRange : GraphicsRectangle {
  GraphicsNumberRange() {};
  GraphicsNumberRange(float from, float to, int numCnt, float bX, float bY, float tX, float tY)
    : GraphicsRectangle(bX, bY, tX, tY), from(from), to(to), numberCount(numCnt) {};

  float from = 0.0f;
  float to = 0.0f;
  int numberCount = 0;
  QString font = "Arial";
};

/**
 * @brief This class implements a visualizer for time-frequency analysis.
 *
 * Every time this control gets updated the whole surface gets repainted.
 *
 * This class also handles user keyboard and mouse input. Scrolling related
 * events are skipped and handled by the parent.
 */
class TfVisualizer : public QOpenGLWidget {
Q_OBJECT

public:
		explicit TfVisualizer(QWidget *parent = nullptr);
		~TfVisualizer() override;
    void setDataToDraw(std::vector<float> values, float xCount, float yCount);
    void setSeconds(int secs);
    void setFrameSize(int fs);

protected:
	void cleanup();
  void initializeGL() override;
	void paintGL() override;
  void resizeGL(int w, int h) override;
  /*void mousePressEvent(QMouseEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;*/

private:
  void createContext();
  void logLastGLMessage();
  std::vector<GLfloat> generateTriangulatedGrid(const std::vector<float> xAxis,
    const std::vector<float> yAxis, const std::vector<float>& values);
  GLuint setupColormapTexture(std::vector<float> colormap);
  std::vector<GLfloat> generateGradient();
  void renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor);
  void renderVertical(const GraphicsNumberRange& range, QColor color);
  void renderHorizontal(const GraphicsNumberRange& range, QColor color);

  void convertToRange(std::vector<float>& values, float newMin, float newMax);
  std::vector<float> generateAxis(int pointCount);
  /**
   * @brief This method is used to skip some code that would break if no file is
   * open and/or the current montage is empty.
   */
  bool ready();

	std::unique_ptr<OpenGLProgram> channelProgram;
	bool paintingDisabled = false;
	GLuint posBuffer;
	std::vector<GLfloat> posBufferData;
  graphics::Colormap colormap;
  GLuint colormapTextureId;

  //spectogram
  const float specBotX = -0.8f;
  const float specTopX = 0.7;
  const float specBotY = -0.7f;
  const float specTopY = 0.8f;
  //TODO: move this to separate class
  const float gradientX = 0.9f;
  float maxGradVal = 0.0f;
  float minGradVal = 0.0f;
  int seconds = 0;
  int frameSize = 0;
  bool gradClicked = false;
};

#endif // TFVISUALIZER_H
