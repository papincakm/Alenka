#ifndef TFVISUALIZER_H
#define TFVISUALIZER_H

#include "openglinterface.h"

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

namespace AlenkaSignal {
class OpenCLContext;
}
class OpenDataFile;
class OpenGLProgram;

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
		void setDataToDraw(const std::vector<std::vector<QVector3D>>& grid);

protected:
	void cleanup();
  void initializeGL() override;
	void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  void createContext();
  void logLastGLMessage();
	std::vector<GLfloat> triangulateGrid(const std::vector<std::vector<QVector3D>>& grid);

  /**
   * @brief Tests whether SignalProcessor is ready to return blocks.
   *
   * This method is used to skip some code that would break if no file is
   * open and/or the current montage is empty.
   */
  bool ready();

	std::unique_ptr<OpenGLProgram> channelProgram;
	bool paintingDisabled = false;
	GLuint posBuffer;
	std::vector<GLfloat> posBufferData;
};

#endif // TFVISUALIZER_H
