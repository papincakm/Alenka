#ifndef ALENKAFILE_ELC_H
#define ALENKAFILE_ELC_H

#include <string>
#include <vector>

namespace AlenkaFile {

struct ElcPosition {
  std::string label = "";
  float x = 0;
  float y = 0;
  float z = 0;
};

  /**
  * @brief Reads elc files.
  */
class ElcFileReader {
public:
  static std::vector<ElcPosition> read(std::string fileName);
};
}



#endif // ALENKAFILE_ELC_H