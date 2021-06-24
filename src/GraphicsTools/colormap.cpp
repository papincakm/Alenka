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

QMenu* Colormap::getPalleteMenu(QWidget* widget) {
  QMenu* menu = new QMenu("Color Pallete", widget);
  auto* palleteGroup = new QActionGroup(widget);
  palleteGroup->setExclusive(true);

  //TODO: lots of copied code, mby find how to do list of qactions
  //set color jet
  QAction* setColorJet = new QAction("Jet", widget);

  widget->connect(setColorJet, &QAction::triggered, [this, widget]() {
    this->changeColorPallete(ColorPallete::Jet);
    widget->update();
  });
  setColorJet->setCheckable(true);

  menu->addAction(setColorJet);
  palleteGroup->addAction(setColorJet);

  //set color rainbow
  QAction* setColorRainbow = new QAction("Rainbow", widget);

  widget->connect(setColorRainbow, &QAction::triggered, [this, widget]() {
    this->changeColorPallete(ColorPallete::Rainbow);
    widget->update();
  });
  setColorRainbow->setCheckable(true);

  menu->addAction(setColorRainbow);
  palleteGroup->addAction(setColorRainbow);

  //set color cool warm smooth
  QAction* setColorCoolWarmSmooth = new QAction("Coolwarm", widget);

  widget->connect(setColorCoolWarmSmooth, &QAction::triggered, [this, widget]() {
    this->changeColorPallete(ColorPallete::CoolWarmSmooth);
    widget->update();
  });
  setColorCoolWarmSmooth->setCheckable(true);

  menu->addAction(setColorCoolWarmSmooth);
  palleteGroup->addAction(setColorCoolWarmSmooth);

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

  for (int i = 0; i < colormapTextureBuffer.size(); i += 4) {
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

void Colormap::setPartitionCount(int count) {
  partitionCount = count;
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
  }
}

void Colormap::createTextureBR() {
  std::vector<float> colorTemplate = getColorTemplate(currentPallete);

  int colorCnt = colorTemplate.size() / 3.0f;
  int interpolationRegions = colorCnt - 1;

  std::vector<int> colorPosition(colorCnt);
  colorPosition[0] = 0;
  for (int i = 1; i < colorCnt; i++) {
    colorPosition[i] = i * partitionCount / (interpolationRegions) - 1;
  }

  //movable center
  for (int i = 1; i < colorCnt - 1; i++) {
    colorPosition[i] = truncate<float>(colorPosition[i] + center, 1, partitionCount - 1);
  }

  int red = 0;
  int green = 1;
  int blue = 2;

  /*std::cout << "colorPos: ";
  for (auto a : colorPosition) {
    std::cout << a << " ";
  }
  std::cout << "\n";*/

  //TODO: is rgba required or is rgb sufficient? 
  std::vector<float> colormap(partitionCount * 4, 0);

  //interpolate with next color
  for (int i = 0; i < interpolationRegions; i++) {
    int firstColor = i * 3;
    int secondColor = firstColor + 3;

    float parts = colorPosition[i + 1] - colorPosition[i];
    for (int j = 0; j <= parts; j++) {
      float redC = (parts - j) / parts * colorTemplate[firstColor + red] + j /
        parts * colorTemplate[secondColor + red];
      float greenC = (parts - j) / parts * colorTemplate[firstColor + green] + j /
        parts * colorTemplate[secondColor + green];
      float blueC = (parts - j) / parts * colorTemplate[firstColor + blue] + j /
        parts * colorTemplate[secondColor + blue];

      int pos = colorPosition[i] * 4 + j * 4;
      colormap[pos + red] = redC;
      colormap[pos + green] = greenC;
      colormap[pos + blue] = blueC;
      colormap[pos + 3] = 1.0f;
    }
  }
  
  colormapTextureBuffer = colormap;
  defaultColormapTextureBuffer = std::move(colormap);
}