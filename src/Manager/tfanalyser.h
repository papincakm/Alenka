#ifndef TFANALYSER_H
#define TFANALYSER_H

#include "../../Alenka-Signal/include/AlenkaSignal/fftprocessor.h"

#include <unsupported/Eigen/FFT>

#include <QWidget>
#include <QtWidgets>
#include <QColor>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <memory>


#include <vector>

class OpenDataFile;
class TfVisualizer;

//TODO: copied from filterprocessor, this should be in separate filter function file
template <class T> T hammingWindow(int n, int M) {
  const double tmp = 2 * M_PI * n / (M - 1);
  return static_cast<T>(0.54 - 0.46 * cos(tmp));
}

template <class T> T blackmanWindow(int n, int M) {
  const double a = 0.16, a0 = (1 - a) / 2, a1 = 0.5, a2 = a / 2,
    tmp = 2 * M_PI * n / (M - 1);
  return static_cast<T>(a0 - a1 * cos(tmp) + a2 * cos(2 * tmp));
}

namespace AlenkaSignal {
  class FftProcessor;
} // namespace AlenkaSignal

/**
* @brief Implements 2D scalp map.
*/
class TfAnalyser : public QWidget {
	Q_OBJECT
public:
	explicit TfAnalyser(QWidget *parent = nullptr);

	/**
	* @brief Notifies this object that the DataFile changed.
	* @param file Pointer to the data file. nullptr means file was closed.
	*/
	void changeFile(OpenDataFile *file);

private:
  int parallelQueues = 0;
  int channelToDisplay = 0;
  int secondsToDisplay = 10;
  int filterWindow = 1;
  int frameSize = 128;
  int hopSize = 16;
  bool freeze = true;

  std::unique_ptr<AlenkaSignal::FftProcessor> fftProcessor;
  TfVisualizer *visualizer;
  OpenDataFile *file = nullptr;
  std::vector<QMetaObject::Connection> connections;
  std::unique_ptr<Eigen::FFT<float>> fft;
  QLineEdit* frameLine;
  QLineEdit* hopLine;
  QSpinBox* channelSpinBox;

  void updateConnections();
  bool ready();
  void applyWindowFunction(std::vector<float>& data);

private slots:
	void updateSpectrum();
  void setFrameSize();
  void setHopSize();
  void setChannelToDisplay(int ch);
  void setSecondsToDisplay(int s);
  void setFreezeSpectrum(bool f);
  void setFilterWindow(int fw);
};

#endif // TFANALYSER_H
