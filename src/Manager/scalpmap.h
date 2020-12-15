#ifndef SCALPMAP_H
#define SCALPMAP_H

#include <QWidget>
#include <QPixmap>
#include <QHBoxLayout>
#include <QLabel>

#include <vector>

namespace AlenkaFile {
	struct Event;
}

/**
* @brief Implements 2D scalp map.
*/
class ScalpMap : public QWidget {
	Q_OBJECT

	std::vector<QMetaObject::Connection> connections;

	QLabel* imageLabel;
	QHBoxLayout* scalpLayout;

public:
	explicit ScalpMap(QWidget *parent = nullptr);
};

#endif // SCALPMAP_H
