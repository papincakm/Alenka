#include "colormap.h"

#include <iostream>

using namespace graphics;

//source: https://www.cs.rit.edu/~ncs/color/t_convert.html

/*void rgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
  float min, max, delta;

  min = std::min(r, g, b);
  max = std::max(r, g, b);
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

void hsvToRgb(float& r, float& g, float& b, float h, float s, float v)
{
  int i;
  float f, p, q, t;

  if (s == 0) {
    // achromatic (grey)
    r = g = b = v;
    return;
  }

  h /= 60;			// sector 0 to 5
  i = floor(h);
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

}*/


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

Colormap::Colormap() {
  colormapTextureBuffer = createTextureBR();
}

Colormap::Colormap(ColorPallete colpal) {
  currentPallete = colpal;
  colormapTextureBuffer = createTextureBR();
}

const std::vector<float>& Colormap::get() {
  return colormapTextureBuffer;
}

void Colormap::changeColorPallete(ColorPallete colorPallete) {
  currentPallete = colorPallete;

  colormapTextureBuffer.clear();
  colormapTextureBuffer = createTextureBR();
}

void Colormap::setPartitionCount(int count) {
  partitionCount = count;
}

std::vector<float> Colormap::getColorTemplate(ColorPallete colpal) {
  switch (colpal) {
  case Jet:
    break;
    //return getJetPallete();
  case Rainbow:
    return getRainbowPallete();
  }
}

std::vector<float> Colormap::createTextureBR() {
  std::vector<float> colorTemplate = getColorTemplate(currentPallete);

  for (auto c : colorTemplate) {
    std::cout << c << " ";
  }
  std::cout << "\n";

  int colorCnt = colorTemplate.size() / 3.0f;

  std::vector<float> colormap;

  int red = 0;
  int green = 1;
  int blue = 2;
  float partPerColorCnt = partitionCount / colorCnt;
  for (int i = 0; i < (colorCnt - 1) * 3; i += 3) {
    int firstColor = i;
    int secondColor = i + 3;

    colormap.push_back(colorTemplate[firstColor + red]);
    colormap.push_back(colorTemplate[firstColor + green]);
    colormap.push_back(colorTemplate[firstColor + blue]);
    colormap.push_back(1.0f);

    for (int j = 1; j < partPerColorCnt; j++) {
      float redC = (partPerColorCnt - j) / partPerColorCnt * colorTemplate[firstColor + red] + j /
                   partPerColorCnt * colorTemplate[secondColor + red];
      float greenC = (partPerColorCnt - j) / partPerColorCnt * colorTemplate[firstColor + green] + j /
                     partPerColorCnt * colorTemplate[secondColor + green];
      float blueC = (partPerColorCnt - j) / partPerColorCnt * colorTemplate[firstColor + blue] + j / 
                    partPerColorCnt * colorTemplate[secondColor + blue];

      colormap.push_back(redC);
      colormap.push_back(greenC);
      colormap.push_back(blueC);
      colormap.push_back(1.0f);
    }
  }

  float lastColor = (colorCnt - 1) * 3;
  colormap.push_back(colorTemplate[lastColor + red]);
  colormap.push_back(colorTemplate[lastColor + green]);
  colormap.push_back(colorTemplate[lastColor + blue]);
  colormap.push_back(1.0f);

  return colormap;
}