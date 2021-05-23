#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>
#include <algorithm>
#include <QMenu>

namespace graphics{

enum ColorPallete { Jet, Rainbow, CoolWarmSmooth };

class Colormap {

  int partitionCount = 150;
  int center = 0.0f;
  //TODO: temp solution,
  //possible better sol would be creating graphics widget as parent of tfvis and scalpcanvas
  ColorPallete currentPallete = CoolWarmSmooth;
  std::vector<float> defaultColormapTextureBuffer;
  std::vector<float> colormapTextureBuffer;

public:
  bool changed = false;

  Colormap();
  Colormap(ColorPallete colpal);
  //TODO: temp sol
  void setPartitionCount(int count);
  void changeColorPallete(ColorPallete colorPallete);
  void change(float contrast, float brightness);
  const std::vector<float>& get();
  QMenu* getColormapMenu(QWidget* widget);

private:
  void createTextureBR();
  std::vector<float> getColorTemplate(ColorPallete colpal);
  QMenu* getPalleteMenu(QWidget* widget);

  //color palletes
  std::vector<float> getJetPallete();
  std::vector<float> getRainbowPallete();
  std::vector<float> getCoolWarmSmoothPallete();
};

}

#endif // COLORMAP_H