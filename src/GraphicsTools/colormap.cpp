#include "colormap.h"

#include <iostream>

using namespace graphics;

//source: https://www.cs.rit.edu/~ncs/color/t_convert.html

void rgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
  float min, max, delta;

  min = std::min({ r, g, b });
  max = std::max({ r, g, b });
  v = max;				// v

  delta = max - min;

  if (max != 0)
    s = delta / max;		// s
  else {
    // r = g = b = 0		// s = 0, v is undefined
    s = 0;
    h = -1;
    return;
  }

  if (r == max)
    h = (g - b) / delta;		// between yellow & magenta
  else if (g == max)
    h = 2 + (b - r) / delta;	// between cyan & yellow
  else
    h = 4 + (r - g) / delta;	// between magenta & cyan

  h *= 60;				// degrees
  if (h < 0)
    h += 360;
}

void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
{
  int i;
  float f, p, q, t;

  if (s == 0) {
    // achromatic (grey)
    r = g = b = v;
    return;
  }

  h /= 60;			// sector 0 to 5
  i = std::floor(h);
  f = h - i;			// factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));

  switch (i) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  default:		// case 5:
    r = v;
    g = p;
    b = q;
    break;
  }

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

//TODO: opengl prints flipped colormap
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
}

float truncate(float value)
{
  if (value < 0) return 0.0f;
  if (value > 1.0f) return 1.0f;

  return value;
}

void Colormap::change(float contrast, float brightness) {
  std::cout << "changeSat\n";

  center = brightness;
  createTextureBR();

  /*for (int i = 0; i < colormapTextureBuffer.size(); i += 4) {
    colormapTextureBuffer[i] = truncate(defaultColormapTextureBuffer[i] * contrast);
    colormapTextureBuffer[i + 1] = truncate(defaultColormapTextureBuffer[i + 1] * contrast);
    colormapTextureBuffer[i + 2] = truncate(defaultColormapTextureBuffer[i + 2] * contrast);
  }*/ 

  std::cout << "changeSatEND factor is\n";
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
    colorPosition[i] += center;
  }

  int red = 0;
  int green = 1;
  int blue = 2;

  //TODO: what to with remainders
  /*for (int i = 0; i < colorCnt / 2; i++) {
    partPerColorCnt[i] = partitionCount / colorCnt - (center / colorCnt / 2);
  }

  for (int i = colorCnt / 2; i < colorCnt; i++) {
    partPerColorCnt[i] = partitionCount / colorCnt + (center / colorCnt / 2);
  }*/

  std::cout << "colorPos: ";
  for (auto a : colorPosition) {
    std::cout << a << " ";
  }
  std::cout << "\n";

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