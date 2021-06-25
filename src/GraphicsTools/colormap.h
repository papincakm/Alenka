#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>
#include <algorithm>
#include <QMenu>

#define BRIGHTNESS_MAX 149.0f
#define BRIGHTNESS_MIN -149.0f
#define COLOR_PARTITIONS 150
#define CONTRAST_MAX 100.0f
#define CONTRAST_MIN 1.0f

namespace graphics{

// How to add new colormap
// 1. Add new entry to ColorPallete enum
// 2. Add new get*Colormap*pallete() function
// 3. Add new entry to menu in getPalleteMenu
// 4. Add new entry to getCollorPallete()

enum ColorPallete { Jet, Rainbow, CoolWarmSmooth, Inferno, Kindlmann, ExtendedKindlmann };

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
  QAction* getColormapAction(QWidget* widget, QString colorS, ColorPallete colorP);

  //color palletes
  std::vector<float> getJetPallete();
  std::vector<float> getRainbowPallete();
  std::vector<float> getCoolWarmSmoothPallete();
  std::vector<float> getInfernoPallete();
  std::vector<float> getKindlmannPallete();
  std::vector<float> getExtendedKindlmannPallete();
};

}

#endif // COLORMAP_H