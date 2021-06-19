#include "trackmanager.h"

#include "../../Alenka-File/include/AlenkaFile/datafile.h"
#include "../DataModel/opendatafile.h"
#include "../DataModel/undocommandfactory.h"
#include <fstream>
#include <sstream>

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

//TODO: move this to separate file and refactor
//TODO: treat errors
void TrackManager::loadCoordinates() {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Load Electrode Locations"), "/", "Electrode location files (*.elc)");

		//TODO: move to separate file
		std::ifstream f(fileName.toStdString());

		if (f.is_open()) {
				std::vector<std::pair<std::string, QVector3D>> positions;
				std::string line;
				int nPositions;

				//get positions count
				while (std::getline(f, line)) {
						if ((line.find("NumberPositions=") != std::string::npos)) {
								std::istringstream iss(line);
								std::string data;
								iss >> data;
								iss >> nPositions;
								break;
						}
				}

				//get positions
				while (std::getline(f, line)) {
						if ((line.find("Positions") != std::string::npos)) {
								break;
						}
				}

				//load positions
				for (int i = 0; i < nPositions; i++) {
						std::getline(f, line);
						std::istringstream iss(line);

						std::string label = "";
						std::string loaded;
						float posX;
						float posY;
						float posZ;

            int ll = 0;
            while (++ll) {
              iss >> loaded;

              if (loaded == ":")
                break;

              if (ll > 1)
                label.append(" ");

              label.append(loaded);
            }

						iss >> posX >> posY >> posZ;

						std::transform(label.begin(), label.end(), label.begin(), ::tolower);

						positions.push_back(std::make_pair(label, QVector3D(posX, posY, posZ)));
				}

				int row = tableView->model()->rowCount();
				int col = tableView->model()->columnCount();

				const int tableLabel = 1;
				const int tableHidden = 5;
				const int tableX = 6;
				const int tableY = 7;
				const int tableZ = 8;

				for (int i = 0; i < row; i++) {
						bool found = false;
						std::string label = tableView->model()->data(tableView->model()->index(i, tableLabel), Qt::DisplayRole).toString().toStdString();
						std::transform(label.begin(), label.end(), label.begin(), ::tolower);

            bool tableViewBlockState;
            if (i < row - 1) {
              tableViewBlockState = tableView->model()->blockSignals(true);
            }

						for (auto v : positions) {
								if ((label.compare(v.first)) == 0) {
										tableView->model()->setData(tableView->model()->index(i, tableX), v.second.x());
										tableView->model()->setData(tableView->model()->index(i, tableY), v.second.y());
										tableView->model()->setData(tableView->model()->index(i, tableZ), v.second.z());
										found = true;
										break;
								}
						}
						if (!found)
								tableView->model()->setData(tableView->model()->index(i, tableHidden), true);

            if (i < row - 1) {
              tableView->model()->blockSignals(tableViewBlockState);
            }
				}
		}


}