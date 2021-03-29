#ifndef TFANALYSER_H
#define TFANALYSER_H

#include <unsupported/Eigen/FFT>

#include <QWidget>
#include <QColor>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <memory>


#include <vector>

class OpenDataFile;
class TfVisualizer;

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
		int channelToDisplay = 0;
		int secondToDisplay = 2;
		int frameSize = 512;
		int hopSize = 500;

		std::vector<float> buffer;
	  TfVisualizer *visualizer;
		OpenDataFile *file = nullptr;
		std::vector<QMetaObject::Connection> connections;
		std::unique_ptr<Eigen::FFT<float>> fft;
		std::vector<std::vector<QVector3D>> spectrumMatrix;


		void updateConnections();

private slots:
	void updateSpectrum();
};

#endif // TFANALYSER_H
