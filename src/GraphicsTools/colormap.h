#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>
#include <algorithm>

namespace graphics{

enum ColorPallete { Jet, Rainbow, CoolWarmSmooth };

class Colormap {

  int partitionCount = 150;
  int center = 0.0f;
  ColorPallete currentPallete = CoolWarmSmooth;
  std::vector<float> defaultColormapTextureBuffer;
  std::vector<float> colormapTextureBuffer;

public:
  Colormap();
  Colormap(ColorPallete colpal);
  void setPartitionCount(int count);
  void changeColorPallete(ColorPallete colorPallete);
  void change(float contrast, float brightness);
  const std::vector<float>& get();

private:
  void createTextureBR();
  std::vector<float> getColorTemplate(ColorPallete colpal);

  //color palletes
  std::vector<float> getJetPallete();
  std::vector<float> getRainbowPallete();
  std::vector<float> getCoolWarmSmoothPallete();
};

}

#endif // COLORMAP_H