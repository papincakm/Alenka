#include "trackmanager.h"

#include "../../Alenka-File/include/AlenkaFile/datafile.h"
#include "../../Alenka-File/include/AlenkaFile/elc.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/undocommandfactory.h"

#include<math.h>
#include<iostream>

using namespace AlenkaFile;

TrackManager::TrackManager(QWidget *parent) : Manager(parent) {
	addSeparator();
	auto loadCoordinatesAction = new QAction("Load Electrode Locations", this);
	loadCoordinatesAction->setStatusTip("Load electrode locations from file.");
	connect(loadCoordinatesAction, SIGNAL(triggered()), this, SLOT(loadCoordinates()));
	tableView->addAction(loadCoordinatesAction);
}

bool TrackManager::insertRowBack() {
  const int selected = OpenDataFile::infoTable.getSelectedMontage();

  // Montage with index 0 shouldn't be edited.
  if (file && 0 != selected) {
    auto montageTable = file->dataModel->montageTable();
    assert(0 < montageTable->rowCount());

    const int rc = montageTable->trackTable(selected)->rowCount();
    file->undoFactory->insertTrack(selected, rc, 1, "add Track row");

    return true;
  }

  return false;
}

void TrackManager::loadCoordinates() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Load Electrode Locations"), "/", "Electrode location files (*.elc)");
  std::vector<ElcPosition> positions = ElcFileReader::read(fileName.toStdString());

  if (!positions.empty()) {
    int row = tableView->model()->rowCount();

    const int tableLabel = 1;
    const int tableX = 6;
    const int tableY = 7;
    const int tableZ = 8;

    bool vitnessBlockState;
    if (trackTable) {
      vitnessBlockState = VitnessTrackTable::vitness(trackTable)->blockSignals(true);
    }

    for (int i = 0; i < row; i++) {
      bool found = false;
      std::string label = tableView->model()->data(tableView->model()->index(i, tableLabel), Qt::DisplayRole).toString().toStdString();
      std::transform(label.begin(), label.end(), label.begin(), ::tolower);

      for (auto v : positions) {
        if ((label.compare(v.label)) == 0) {
          tableView->model()->setData(tableView->model()->index(i, tableX), v.x);
          tableView->model()->setData(tableView->model()->index(i, tableY), v.y);

          if (i == row - 1 && trackTable) {
            VitnessTrackTable::vitness(trackTable)->blockSignals(vitnessBlockState);
          }

          tableView->model()->setData(tableView->model()->index(i, tableZ), v.z);
          found = true;
          break;
        }
      }
      if (!found) {
        tableView->model()->setData(tableView->model()->index(i, tableX), NAN);
        tableView->model()->setData(tableView->model()->index(i, tableY), NAN);

        if (i == row - 1 && trackTable) {
          VitnessTrackTable::vitness(trackTable)->blockSignals(vitnessBlockState);
        }

        tableView->model()->setData(tableView->model()->index(i, tableZ), NAN);
      }
    }
  }
  else {
    int ret = QMessageBox::critical(this, tr("Alenka"),
      tr("Invalid structure of the file.\n"));
  }
}
