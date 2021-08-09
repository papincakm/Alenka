#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "manager.h"
#include "../DataModel/vitnessdatamodel.h"

/**
 * @brief Track manager widget implementation.
 */
class TrackManager : public Manager {
  Q_OBJECT

public:
	explicit TrackManager(QWidget *parent = nullptr);
  void setTrackTable(AlenkaFile::AbstractTrackTable* trackTable) { this->trackTable = trackTable; };
protected:
  AlenkaFile::AbstractTrackTable* trackTable;

protected slots:
  bool insertRowBack() override;
	void loadCoordinates();
};

#endif // TRACKMANAGER_H
