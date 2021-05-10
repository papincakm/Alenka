#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>
#include <algorithm>

namespace graphics {
enum ColorPallete{ Jet, Rainbow };

class Colormap {

  int partitionCount = 150;
  ColorPallete currentPallete = Rainbow;
  std::vector<float> colormapTextureBuffer;

public:
  Colormap();
  Colormap(ColorPallete colpal);
  void setPartitionCount(int count);
  void changeColorPallete(ColorPallete colorPallete);
  const std::vector<float>& get();

private:
  std::vector<float> createTextureBR();
  std::vector<float> getColorTemplate(ColorPallete colpal);

  //color palletes
  std::vector<float> getJetPallete();
  std::vector<float> getRainbowPallete();
};

}

#endif // COLORMAP_H
