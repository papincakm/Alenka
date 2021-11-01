#include "colormap.h"

#include <iostream>
#include <cmath>

using namespace graphics;

QMenu* Colormap::getColormapMenu(QWidget* widget) {
  QMenu* menu = new QMenu("Colormap", widget);

  //setup pallete
  menu->addMenu(getPalleteMenu(widget));

  return menu;
}

QAction* Colormap::getColormapAction(QWidget* widget, QString colorS, ColorPallete colorP) {
  QAction* colorAction = new QAction(colorS, widget);

  widget->connect(colorAction, &QAction::triggered, [this, widget, colorP]() {
    this->changeColorPallete(colorP);
    widget->update();
  });
  colorAction->setCheckable(true);

  return colorAction;
}

QMenu* Colormap::getPalleteMenu(QWidget* widget) {
  QMenu* menu = new QMenu("Color Pallete", widget);
  auto* palleteGroup = new QActionGroup(widget);
  palleteGroup->setExclusive(true);

  //TODO: lots of copied code, mby find how to do list of qactions
  //set color jet
  QAction* setColorJet = getColormapAction(widget, "Jet", ColorPallete::Jet);
  menu->addAction(setColorJet);
  palleteGroup->addAction(setColorJet);

  //set color rainbow
  QAction* setColorRainbow = getColormapAction(widget, "Rainbow", ColorPallete::Rainbow);
  menu->addAction(setColorRainbow);
  palleteGroup->addAction(setColorRainbow);

  //set color cool warm smooth
  QAction* setColorCoolWarmSmooth = getColormapAction(widget, "Coolwarm", ColorPallete::CoolWarmSmooth);
  menu->addAction(setColorCoolWarmSmooth);
  palleteGroup->addAction(setColorCoolWarmSmooth);

  //set color inferno
  QAction* setColorInferno = getColormapAction(widget, "Inferno", ColorPallete::Inferno);
  menu->addAction(setColorInferno);
  palleteGroup->addAction(setColorInferno);

  //set color Kindlmann
  QAction* setColorKindlmann = getColormapAction(widget, "Kindlmann", ColorPallete::Kindlmann);
  menu->addAction(setColorKindlmann);
  palleteGroup->addAction(setColorKindlmann);

  //set color ExtendedKindlmann
  QAction* setColorExtendedKindlmann = getColormapAction(widget, "ExtendedKindlmann",
    ColorPallete::ExtendedKindlmann);
  menu->addAction(setColorExtendedKindlmann);
  palleteGroup->addAction(setColorExtendedKindlmann);


  switch (currentPallete) {
  case ColorPallete::Jet:
    setColorJet->setChecked(true);
    break;
  case ColorPallete::Rainbow:
    setColorRainbow->setChecked(true);
    break;
  case ColorPallete::CoolWarmSmooth:
    setColorCoolWarmSmooth->setChecked(true);
    break;
  case ColorPallete::Inferno:
    setColorInferno->setChecked(true);
    break;
  case ColorPallete::Kindlmann:
    setColorInferno->setChecked(true);
    break;
  case ColorPallete::ExtendedKindlmann:
    setColorInferno->setChecked(true);
    break;
  }

  return menu;
}

std::vector<float> Colormap::getRainbowPallete() {
  return
  {
    0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f
  };
}

std::vector<float> Colormap::getJetPallete() {
  return 
  {
    0.0f, 0.0f, 0.6f,
    0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.6f, 0.0f, 0.0f
  };
}

std::vector<float> Colormap::getCoolWarmSmoothPallete() {
  return
  {
    59.0f / 255.0f, 76.0f / 255.0f, 192.0f / 255.0f,
    221.0f / 255.0f, 221.0f / 255.0f, 221.0f / 255.0f,
    180.0f / 255.0f, 4.0f / 255.0f, 38.0f / 255.0f
  };
}

std::vector<float> Colormap::getInfernoPallete() {
  return
  {
    0.0f, 0.0f, 4.0f / 255.0f,
    40.0f / 255.0f, 11.0f / 255.0f, 84.0f / 255.0f,
    101.0f / 255.0f, 21.0f / 255.0f, 110.0f / 255.0f,
    159.0f / 255.0f, 42.0f / 255.0f, 99.0f / 255.0f,
    212.0f / 255.0f, 72.0f / 255.0f, 66.0f / 255.0f,
    245.0f / 255.0f, 125.0f / 255.0f, 21.0f / 255.0f,
    250.0f / 255.0f, 193.0f / 255.0f, 39.0f / 255.0f,
    252.0f / 255.0f, 255.0f / 255.0f, 164.0f / 255.0f,
  };
}

std::vector<float> Colormap::getKindlmannPallete() {
  return
  {
    0.0f, 0.0f, 0.0f,
    39.0f / 255.0f, 4.0f / 255.0f, 82.0f / 255.0f,
    24.0f / 255.0f, 8.0f / 255.0f, 163.0f / 255.0f,
    7.0f / 255.0f, 69.0f / 255.0f, 142.0f / 255.0f,
    5.0f / 255.0f, 105.0f / 255.0f, 105.0f / 255.0f,
    7.0f / 255.0f, 137.0f / 255.0f, 66.0f / 255.0f,
    15.0f / 255.0f, 168.0f / 255.0f, 8.0f / 255.0f,
    97.0f / 255.0f, 193.0f / 255.0f, 9.0f / 255.0f,
    205.0f / 255.0f, 205.0f / 255.0f, 10.0f / 255.0f,
    251.0f / 255.0f, 210.0f / 255.0f, 163.0f / 255.0f,
    253.0f / 255.0f, 232.0f / 255.0f, 223.0f / 255.0f,
    255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f
  };
}

std::vector<float> Colormap::getExtendedKindlmannPallete() {
  return
  {
    0.0f, 0.0f, 0.0f,
    29.0f / 255.0f, 3.0f / 255.0f, 65.0f / 255.0f,
    8.0f / 255.0f, 6.0f / 255.0f, 119.0f / 255.0f,
    4.0f / 255.0f, 49.0f / 255.0f, 87.0f / 255.0f,
    3.0f / 255.0f, 72.0f / 255.0f, 61.0f / 255.0f,
    4.0f / 255.0f, 92.0f / 255.0f, 26.0f / 255.0f,
    22.0f / 255.0f, 110.0f / 255.0f, 5.0f / 255.0f,
    88.0f / 255.0f, 121.0f / 255.0f, 6.0f / 255.0f,
    165.0f / 255.0f, 120.0f / 255.0f, 8.0f / 255.0f,
    246.0f / 255.0f, 98.0f / 255.0f, 58.0f / 255.0f,
    249.0f / 255.0f, 125.0f / 255.0f, 135.0f / 255.0f,
    250.0f / 255.0f, 144.0f / 255.0f, 225.0f / 255.0f,
    225.0f / 255.0f, 182.0f / 255.0f, 251.0f / 255.0f,
    227.0f / 255.0f, 209.0f / 255.0f, 253.0f / 255.0f,
    233.0f / 255.0f, 234.0f / 255.0f, 254.0f / 255.0f,
    255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f,
  };
}


Colormap::Colormap() {
  createTextureBR();
}

Colormap::Colormap(ColorPallete colpal) {
  currentPallete = colpal;
  createTextureBR();
}

const std::vector<float>& Colormap::get() {
  return colormapTextureBuffer;
}

void Colormap::changeColorPallete(ColorPallete colorPallete) {
  currentPallete = colorPallete;

  colormapTextureBuffer.clear();
  createTextureBR();
  changed = true;
}

template <typename T>
T truncate(T value, T min, T max)
{
  if (value < min) return min;
  if (value > max) return max;

  return value;
}

void Colormap::change(float contrast, float brightness) {
  center = brightness;
  createTextureBR();

  for (int i = 0; i < colormapTextureBuffer.size(); i += 3) {
    colormapTextureBuffer[i] = truncate<float>(defaultColormapTextureBuffer[i] * contrast, 0, 1.0f);
    colormapTextureBuffer[i + 1] = truncate<float>(defaultColormapTextureBuffer[i + 1] * contrast, 0, 1.0f);
    colormapTextureBuffer[i + 2] = truncate<float>(defaultColormapTextureBuffer[i + 2] * contrast, 0, 1.0f);
  }

  changed = true;
}

void Colormap::reset() {
  center = 0.0f;
  createTextureBR();

  changed = true;
}

std::vector<float> Colormap::getColorTemplate(ColorPallete colpal) {
  switch (colpal) {
  case Jet:
    return getJetPallete();
    break;
  case Rainbow:
    return getRainbowPallete();
    break;
  case CoolWarmSmooth:
    return getCoolWarmSmoothPallete();
    break;
  case Inferno:
    return getInfernoPallete();
    break;
  case Kindlmann:
    return getKindlmannPallete();
    break;
  case ExtendedKindlmann:
    return getExtendedKindlmannPallete();
    break;
  }
}

void Colormap::createTextureBR() {
  std::vector<float> colorTemplate = getColorTemplate(currentPallete);

  int colorCnt = colorTemplate.size() / 3.0f;
  int interpolationRegions = colorCnt - 1;

  std::vector<int> colorPosition(colorCnt);
  colorPosition[0] = 0;

  for (int i = 1; i < colorCnt; i++) {
    colorPosition[i] = i * COLOR_PARTITIONS / (interpolationRegions) - 1;
  }

  //movable center
  for (int i = 1; i < colorCnt - 1; i++) {
    colorPosition[i] = truncate<float>(colorPosition[i] + center, 0, COLOR_PARTITIONS - 1);
  }

  int red = 0;
  int green = 1;
  int blue = 2;
  int partitionSize = 3;

  std::vector<float> colormap(COLOR_PARTITIONS * partitionSize, 0);

  //interpolate with next color
  for (int i = 0; i < interpolationRegions; i++) {
    int firstColor = i * 3;
    int secondColor = firstColor + 3;

    float parts = colorPosition[i + 1] - colorPosition[i];
    if (parts == 0)
      continue;

    for (int j = 0; j <= parts; j++) {
      float redC = (parts - j) / parts * colorTemplate[firstColor + red] + j /
        parts * colorTemplate[secondColor + red];
      float greenC = (parts - j) / parts * colorTemplate[firstColor + green] + j /
        parts * colorTemplate[secondColor + green];
      float blueC = (parts - j) / parts * colorTemplate[firstColor + blue] + j /
        parts * colorTemplate[secondColor + blue];

      int pos = colorPosition[i] * partitionSize + j * partitionSize;
      colormap[pos + red] = redC;
      colormap[pos + green] = greenC;
      colormap[pos + blue] = blueC;
    }
  }
  
  colormapTextureBuffer = colormap;
  defaultColormapTextureBuffer = std::move(colormap);
}