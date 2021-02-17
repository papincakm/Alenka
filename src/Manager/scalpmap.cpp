#include "scalpmap.h"

#include "../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include "../scalpcanvas.h"
#include <elidedlabel.h>
#include <helplink.h>
#include <QVBoxLayout>


#include <sstream>

#include <QtWidgets>

using namespace AlenkaFile;
using namespace std;


ScalpMap::ScalpMap(QWidget *parent) : QWidget(parent) {
	connect(&OpenDataFile::infoTable, SIGNAL(selectedMontageChanged(int)), this,
		SLOT(updateConnections(int)));

	auto box = new QVBoxLayout;
	setLayout(box);
	box->setContentsMargins(0, 0, 0, 0);
	box->setSpacing(0);

	scalpCanvas = new ScalpCanvas(this);
	box->addWidget(scalpCanvas);
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateConnections(int row) {
	for (auto e : trackConnections)
		disconnect(e);
	trackConnections.clear();

	if (file) {
		const AbstractMontageTable *mt = file->dataModel->montageTable();

		if (0 <= row && row < mt->rowCount()) {
			auto vitness = VitnessTrackTable::vitness(mt->trackTable(row));

			auto c = connect(vitness, SIGNAL(valueChanged(int, int)), this, SLOT(updateLabels()));
			trackConnections.push_back(c);

			c = connect(vitness, SIGNAL(rowsInserted(int, int)), this, SLOT(updateLabels()));

			trackConnections.push_back(c);
			c = connect(vitness, SIGNAL(rowsRemoved(int, int)), this,
				SLOT(updateLabels()));
			trackConnections.push_back(c);

			c = connect(&file->infoTable, SIGNAL(positionChanged(int, double)), this,
				SLOT(updateSpectrum()));
			trackConnections.push_back(c);

			updateLabels();
		}
	}
}

//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
//which will be referenced in here and trackLabelBar
void ScalpMap::updateLabels() {
	labels.clear();
	colors.clear();
	positions.clear();

	if (!file || file->dataModel->montageTable()->rowCount() <= 0)
		return;

	const AbstractTrackTable *trackTable =
		file->dataModel->montageTable()->trackTable(
			OpenDataFile::infoTable.getSelectedMontage());

	if (trackTable->rowCount() <= 0)
		return;

	int hidden = 0;
	int track = 0;

	for (int i = 0; i < trackTable->rowCount(); ++i) {
		assert(track + hidden == i);
		Track t = trackTable->row(i);

		if (t.hidden == false) {
			//TODO: labels, colors and positions should be in one class and one vector
			labels.push_back(QString::fromStdString(t.label));
			colors.push_back(DataModel::array2color<QColor>(t.color));
			positions.push_back(QVector3D(t.x, t.y, t.z));

			++track;
		}
		else {
			++hidden;
		}
	}

	updatePositionsProjected();

	//TODO: refactor this
	scalpCanvas->resetTracks();
	for (int i = 0; i < positionsProjected.size(); i++) {
		try {
			scalpCanvas->addTrack(labels[i], positionsProjected[i]);
		}
		catch (exception) {
		}
	}

	scalpCanvas->setChannelLabels(labels);
	scalpCanvas->setChannelPositions(positionsProjected);

	update();
}

void ScalpMap::updateSpectrum() {
	if (!file || !scalpCanvas)
		return;

	//assert(channelToDisplay < static_cast<int>(file->file->getChannelCount()));

	const int position = OpenDataFile::infoTable.getPosition();

	std::vector<float> channelDataBuffer(file->file->getChannelCount());

	file->file->readSignal(channelDataBuffer.data(), position, position);

	assert(static_cast<int>(channelDataBuffer.size()) == static_cast<int>(file->file->getChannelCount()));

	scalpCanvas->updatePositionFrequencies(channelDataBuffer);
	//update();
}

float degToRad(float n) {
	return n * M_PI / 180;
}

float radToDeg(float n) {
	return n * 180 / M_PI;
}

//TODO: improvised algorithm, find a better way to do this
void ScalpMap::updatePositionsProjected() {
	positionsProjected.clear();

	//compute radius of sphere
	float r = 0;
	for (auto vec : positions) {
		float rCan = std::abs(vec.x());


		rCan = std::abs(vec.x());
		if (rCan > r) {
			r = rCan;
		}

		rCan = std::abs(vec.x());
		if (rCan > r) {
			r = rCan;
		}
	}

	//compute unit sphere
	std::vector<QVector3D> nPos;
	for (auto vec : positions) {
		float n = sqrt(vec.x() * vec.x() + vec.y() * vec.y()
			+ vec.z() * vec.z());
		nPos.push_back(vec / n);
	}

	//compute radiuses
	std::vector<float> rads;
	for (auto vec : nPos) {
		rads.push_back(sqrt(vec.x() * vec.x() + vec.y() * vec.y()
			+ vec.z() * vec.z()));
	}

	//compute thetas
	int i = 0;
	std::vector<float> thetas;
	for (auto vec : nPos) {
		float theta = radToDeg(acos(vec.z() / rads[i]));
		theta = (vec.x() >= 0) ? theta : -1 * theta;
		thetas.push_back(theta);

		i++;
	}

	//compute phis
	std::vector<float> phis;
	for (auto vec : nPos) {
		float phi = radToDeg(atan(vec.y() / vec.x()));
		phi = (vec.x() == 0) ? -1 * phi : phi;
		phis.push_back(phi);
	}

	// project 3D vector to 2D
	i = 0;
	for (auto vec : nPos) {
		float newX = degToRad(thetas[i]) * cos(degToRad(phis[i]));
		float newY = degToRad(thetas[i]) * sin(degToRad(phis[i]));

		positionsProjected.push_back(QVector2D(radToDeg(newX), radToDeg(newY)));
		//positionsProjected.push_back(QVector2D(positions[i].x(), positions[i].y()));
		i++;
	}

	// normalize positions for opengl
	//TODO: this should be in scalpcanvas
	float maxValue = 0;
	for (auto vec : positionsProjected) {
		if (std::abs(vec.x()) > maxValue)
			maxValue = std::abs(vec.x());

		if (std::abs(vec.y()) > maxValue)
			maxValue = std::abs(vec.y());
	}

	//TODO: temp scaling
	maxValue *= 1.5;

	for_each(positionsProjected.begin(), positionsProjected.end(), [maxValue](auto& v)
	{
		v *= 1 / maxValue;
	});
}
