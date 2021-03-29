#include "tfanalyser.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "../tfvisualizer.h"
#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>
//TODO: delete later
#include <iostream>
#include <string>


#include <sstream>

#include <QtWidgets>

using namespace AlenkaFile;


TfAnalyser::TfAnalyser(QWidget *parent) : QWidget(parent), fft(new Eigen::FFT<float>()) {
	auto box = new QVBoxLayout;
	setLayout(box);
	box->setContentsMargins(0, 0, 0, 0);
	box->setSpacing(0);

  /*visualizer = new TfVisualizer(this);
	box->addWidget(visualizer);
	setMinimumHeight(100);
	setMinimumWidth(100);*/
}

void TfAnalyser::changeFile(OpenDataFile *file) {
		this->file = file;
		if (file) {
				updateConnections();
				updateSpectrum();
		}
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void TfAnalyser::updateConnections() {
		return;
		for (auto e : connections)
				disconnect(e);
		connections.clear();

		if (file) {
				auto c = connect(&file->infoTable, SIGNAL(positionChanged(int, double)), this,
						SLOT(updateSpectrum()));
				connections.push_back(c);

				//TODO: should be here?
				/*c = connect(&file->infoTable, SIGNAL(pixelViewWidthChanged(int)),
						SLOT(updateSpectrum()));
				connections.push_back(c);*/
		}
}

//TODO: copied from filtervisualizer
//TODO: make parent class for filter and this that has signal file manipulation methods and menus
void TfAnalyser::updateSpectrum() {
		//spectrumMatrix.clear();
		return;

		if (!file || secondToDisplay == 0 || !this->isVisible())
				return;

		assert(channelToDisplay < static_cast<int>(file->file->getChannelCount()));

		//removeSeries();

		const float fs = file->file->getSamplingFrequency() / 2;

		//axisX->setRange(0, fs);

		const int samplesToUse =
				secondToDisplay * static_cast<int>(file->file->getSamplingFrequency());
		const int position = OpenDataFile::infoTable.getPosition();

		int start = std::max(0, position - samplesToUse / 2);
		int end = start + samplesToUse - 1;
		int sampleCount = end - start + 1; // TODO: Maybe round this to a power of two.

		int frameCount = (sampleCount - frameSize) / hopSize + 1;
		//spectrumMatrix.resize(sampleCount);

		//TODO: problem at eof
		end = start + frameSize - 1;
		sampleCount = frameSize;

		//buffer.resize(sampleCount * file->file->getChannelCount());

		return;

		for (int f = 0; f < frameCount; f++) {

				file->file->readSignal(buffer.data(), start, end);

				auto begin = buffer.begin() + channelToDisplay * sampleCount;
				std::vector<float> input(begin, begin + sampleCount);

				std::vector<std::complex<float>> spectrum;
				fft->fwd(spectrum, input);

				assert(static_cast<int>(input.size()) == sampleCount);
				assert(static_cast<int>(spectrum.size()) == sampleCount);

				float maxVal = 0;

				for (int i = 0; i < sampleCount / 2; i++) {
						float val = std::abs(spectrum[i]);
						if (isfinite(val)) {
								//spectrumSeries->append(fs * i / (sampleCount / 2), val);
								//TODO: x is frame (should be time) and y should be freq bin
								spectrumMatrix[i].push_back(QVector3D(f / frameCount, fs * i / (sampleCount / 2), val));
						}
				}

				start += hopSize;
				end += hopSize;
		}
		
		float maxVal = 0;
		for (int i = 0; i < spectrumMatrix.size(); i++) {
				for (int j = 0; j < spectrumMatrix[i].size(); j++) {
						maxVal = std::max(maxVal, spectrumMatrix[i][j].z());
				}
		}

		for (int i = 0; i < spectrumMatrix.size(); i++) {
				for (int j = 0; j < spectrumMatrix[i].size(); j++) {
						spectrumMatrix[i][j].setZ(spectrumMatrix[i][j].z() / maxVal);
				}
		}

		//visualizer->setDataToDraw(spectrumMatrix);
		//visualizer->update();
}