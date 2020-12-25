#ifndef SCALPMAP_H
#define SCALPMAP_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QVector2D>
#include <QVector3D>


#include <vector>

class OpenDataFile;
class ScalpCanvas;

/**
* @brief Implements 2D scalp map.
*/
class ScalpMap : public QWidget {
	Q_OBJECT

	OpenDataFile *file = nullptr;
	ScalpCanvas *scalpCanvas;
	int selectedTrack = -1;
	//TODO: this is a copy from tracklabel, might want to make a new class trackLabelModel
	//which will be referenced in here and trackLabelBar
	std::vector<QMetaObject::Connection> trackConnections;
	std::vector<QString> labels;
	std::vector<QColor> colors;
	std::vector<QVector3D> positions;
	std::vector<QVector2D> positionsProjected;

public:
	explicit ScalpMap(QWidget *parent = nullptr);

	/**
	* @brief Notifies this object that the DataFile changed.
	* @param file Pointer to the data file. nullptr means file was closed.
	*/
	void changeFile(OpenDataFile *file) { this->file = file; }

private:
	void updatePositionsProjected();

private slots:
	void updateConnections(int row);
	void updateLabels();
};

#endif // SCALPMAP_H
