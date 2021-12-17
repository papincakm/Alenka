#ifndef TFANALYSER_H
#define TFANALYSER_H

#include "../../Alenka-Signal/include/AlenkaSignal/fftprocessor.h"
#include "tfmodel.h"

#include <QWidget>
#include <QtWidgets>
#include <QColor>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <memory>


#include <vector>

#define MAX_FRAME_SIZE 2048
#define MIN_FRAME_SIZE 1
#define MIN_HOP_SIZE 1

class TfVisualizer;

namespace AlenkaSignal {
  class FftProcessor;
} // namespace AlenkaSignal

  /**
  * @brief Recieves signals from global Qt contexts.
  * Controls the TfModel and TfVisualizer to draw a spectrogram.
  */
class TfAnalyser : public QWidget {
  Q_OBJECT

    bool freeze = true;
  bool parentVisible = true;
  bool printTiming = false;

  TfVisualizer* visualizer;
  std::vector<QMetaObject::Connection> connections;
  QLineEdit* frameLine;
  QLineEdit* hopLine;
  QLineEdit* minFreqLine;
  QLineEdit* maxFreqLine;
  QSpinBox* channelSpinBox;
  std::unique_ptr<TfModel> tfModel;

public:
  explicit TfAnalyser(QWidget *parent = nullptr);

  /**
  * @brief Notifies this object that the DataFile changed.
  * @param file Pointer to the data file. nullptr means file was closed.
  */
  void changeFile(OpenDataFile *file);

private:
  void updateConnections();
  bool ready();
  void setupTfVisualizer(QVBoxLayout* mainBox);

  //menus
  QGroupBox* createResolutionMenu();
  QGroupBox* createFrequencyMenu();
  QGroupBox* createFilterMenu();
  QWidget* createFreezeMenu();
  QLayout* createChannelTimeMenu();
  private slots:
  /**
  * @brief Calls FftProcessor to compute FFT and updates the data in TfVisualizer.
  */
  void updateSpectrum();
  void setFrameSize();
  void setHopSize();
  void setMinFreqDraw();
  void setMaxFreqDraw();
  void setChannelToDisplay(int ch);
  void setSecondsToDisplay(int s);
  void setFreezeSpectrum(bool f);
  void setFCompensation(bool f);
  void setLogCompensation(bool l);
  void setFilterWindow(int fw);
  void parentVisibilityChanged(bool vis);
};

#endif // TFANALYSER_H
