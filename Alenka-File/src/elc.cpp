#include "../include/AlenkaFile/elc.h"

#include <fstream>
#include <sstream>
#include <algorithm>

//delete
#include <iostream>

using namespace AlenkaFile;

std::vector<ElcPosition> ElcFileReader::read(std::string fileName) {
  std::vector<ElcPosition> positions;
  std::ifstream f(fileName);

  if (f.is_open()) {
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

      ElcPosition position;

      std::string loaded;

      int ll = 0;
      while (++ll) {
        iss >> loaded;

        if (loaded == ":")
          break;

        if (ll > 1)
          position.label.append(" ");

        position.label.append(loaded);
      }

      iss >> position.x >> position.y >> position.z;

      std::transform(position.label.begin(), position.label.end(), position.label.begin(), ::tolower);
      std::cout << position.label << " x" << position.x << " y" << position.y << " z" << position.z << "\n";
      positions.push_back(position);
    }
  }

  return positions;
}