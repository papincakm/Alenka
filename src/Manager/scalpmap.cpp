#include "scalpmap.h"

#include "../DataModel/opendatafile.h"
#include "../DataModel/vitnessdatamodel.h"
#include <elidedlabel.h>
#include <helplink.h>

#include <sstream>

#include <QtWidgets>

using namespace AlenkaFile;
using namespace std;

ScalpMap::ScalpMap(QWidget *parent) : QWidget(parent), scalpLayout(new QHBoxLayout) {
	imageLabel = new QLabel(this);
	QPixmap pic(":/icons/eeg_head.jpg");

	imageLabel->setGeometry(1, 1, 500, 500);

	int w = imageLabel->width();
	int h = imageLabel->height();

	imageLabel->setPixmap(pic.scaled(w, h, Qt::KeepAspectRatio));
	imageLabel->setScaledContents(true);

	scalpLayout->addWidget(imageLabel);
	this->setLayout(scalpLayout);
}