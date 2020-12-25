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

	scalpCanvas->setChannelPositions(positionsProjected);

	update();
}

//TODO: improvised algorithm, find a better way to do this
void ScalpMap::updatePositionsProjected() {
	positionsProjected.clear();

	// project 3D vector to 2D
	for (auto vec : positions) {
		auto vecCut = QVector2D(vec.x(), vec.y());

		float distFromZero3D = vec.distanceToPoint(QVector3D(0, 0, 0));
		float distFromZero2D = vecCut.distanceToPoint(QVector2D(0, 0));
		
		float project = distFromZero3D / distFromZero2D;

		positionsProjected.push_back(QVector2D(vecCut.x() * project, vecCut.y() * project));
		//positionsProjected.push_back(QVector2D(vecCut.x(), vecCut.y()));
	}

	// move positions to positive values
	/*float minValue = 0;
	for (auto vec : positionsProjected) {
		if (vec.x() < minValue)
			minValue = vec.x();

		if (vec.y() < minValue)
			minValue = vec.y();
	}
	minValue = std::abs(minValue);

	for_each(positionsProjected.begin(), positionsProjected.end(), [minValue](auto& v)
	{
		v.setX(v.x() + minValue);
		v.setY(v.y() + minValue);
	});*/

	// normalize positions for opengl
	float maxValue = 0;
	for (auto vec : positionsProjected) {
		if (std::abs(vec.x()) > maxValue)
			maxValue = std::abs(vec.x());

		if (std::abs(vec.y()) > maxValue)
			maxValue = std::abs(vec.y());
	}

	for_each(positionsProjected.begin(), positionsProjected.end(), [maxValue](auto& v)
	{
		v *= 1 / maxValue;
	});

	/*for (auto vec : positionsProjected) {
		vec.setX(vec.x() / maxValue);
		vec.setY(vec.y() / maxValue);
		//vec.setX(0.2);
		//vec.setY(0.3);
		//vec *= 1/maxValue;
	}*/
}