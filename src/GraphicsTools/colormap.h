#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>
#include <algorithm>
#include <QMenu>

#define COLOR_PARTITIONS 150

namespace graphics{

enum ColorPallete { Jet, Rainbow, CoolWarmSmooth };

class Colormap {
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
  void changeColorPallete(ColorPallete colorPallete);
  void change(float contrast, float brightness);
  void reset();
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