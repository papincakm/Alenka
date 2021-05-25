#include "graphicsrectangle.h"

#include <iostream>

using namespace graphics;

void Rectangle::renderFull() {
  //std::cout << "rectangle renderFull\n";

  QPainter painter(widget);
  QPen pen;  // creates a default pen

  pen.setWidth(1);
  pen.setJoinStyle(Qt::MiterJoin);

  painter.setPen(pen);

  /*painter.setRenderHint(
    QPainter::Antialiasing);*/

  //painter.setWindow(QRect(-1, -1, 1, 1));
  //painter.drawLine(realBotx - 0.6f, realBoty, realBotx - 0.6f, realTopy);
  //painter.drawLine(realTopx + 1.5f, realBoty, realTopx + 1.5f, realTopy);
  painter.drawRect(QRectF(xleftReal, ytopReal, realWidth, realHeight));
}

void Rectangle::calculateWidgetProportions() {
  //TODO: using brute correction to match opengl drawing
  xleftReal = widget->width() / 2.0f + (widget->width() / 2.0f) * xleft - 0.5f;
  xrightReal = widget->width() / 2.0f + (widget->width() / 2.0f) * xright + 0.5f;
  ybotReal = widget->height() / 2.0f + (widget->height() / 2.0f) * ybot * -1 + 0.5f;
  ytopReal = widget->height() / 2.0f + (widget->height() / 2.0f) * ytop * -1 - 0.5f;

  //1.0f is correction to match opengl
  realHeight = abs(ytopReal - ybotReal);// +1.0f;
  realWidth = abs(xrightReal - xleftReal);// +1.0f;
}

void Rectangle::render() {
  //std::cout << "rendering with alignment: " << alignment << "\n";
  switch (alignment) {
  case Alignment::Center:
    renderCenter();
    break;
  case Alignment::Bot:
    renderBot();
    break;
  case Alignment::Top:
    renderTop();
    break;
  default:
    renderFull();
  }
}

void Rectangle::update() {
  calculateWidgetProportions();
}

//TODO: use drawlines ?
void Line::renderTop() {
  //std::cout << " line renderTop\n";
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(xrightReal, ytopReal, xrightReal, ybotReal));
  }
  else {
    //painter.drawLine(realBotx, realTopy, realTopx, realTopy);
    painter.drawLine(QLineF(xleftReal, ytopReal, xrightReal, ytopReal));
  }
}

void Line::renderBot() {
  //std::cout << " line renderBot\n";
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(xleftReal, ytopReal, xleftReal, ybotReal));
  }
  else {
    painter.drawLine(QLineF(xleftReal, ybotReal, xrightReal, ybotReal));
  }
}

void Line::renderCenter() {
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(xleftReal + (xrightReal - xleftReal) / 2.0f, ybotReal, ((xrightReal - xleftReal) / 2.0f), ytopReal));
  }
  else {
    painter.drawLine(QLineF(xleftReal, ybotReal + (ytopReal - ybotReal) / 2.0f, xrightReal, ybotReal + (ytopReal - ybotReal) / 2.0f));
  }
}

void RectangleText::drawText(float x, float y) {
  int fontSize = std::max(8, static_cast<int>(realHeight / 10.0f));
  //std::cout << "text rendering text on y: " << realBoty << " boty is " << boty << " fontsize is : " << fontSize << "\n";
  //text is corrupted if this is not called
  //gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  QPainter painter(widget);
  painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.setRenderHints(QPainter::TextAntialiasing);
  painter.setBackground(backgroundColor);
  painter.setPen(textColor);
  painter.setBrush(textColor);
  painter.setFont(QFont(font, fontSize, QFont::Bold));
  painter.drawText(QRectF(x, y, realWidth, realHeight), text);
  painter.end();
}

//same as renderTop rn
void RectangleText::renderFull() {
  drawText(xleftReal, ytopReal);
}

void RectangleText::renderTop() {
  if (orientation == Orientation::Vertical) {
    drawText(xleftReal, ytopReal);
  }
  else {
    drawText(xrightReal, ytopReal);
  }

}

void RectangleText::renderBot() {
  if (orientation == Orientation::Vertical) {
    drawText(xleftReal, ybotReal);
  }
  else {
    drawText(xleftReal, ytopReal);
  }

}

void RectangleChain::constructObjects() {
  if (orientation == Vertical) {
    constructVertical();
  }
  else {
    constructHorizontal();
  }
}

void RectangleChain::render() {
  for (auto o : objects) {
    //std::cout << "chain item render\n";
    o->render();
  }
}

void RectangleChain::constructVertical() {
  float height = std::fabs(ytop - ybot);
  
  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float biny = height / static_cast<float>(clusterCount - 1);
  float ypos = ybot;

  createObject(0, xleft, xright, ybot, ybot + biny, childOrientation, Bot);
  for (int i = 0; i < clusterCount - 2; i++) {
    ypos += biny;
    createObject(i + 1, xleft, xright, ypos, ypos + biny, childOrientation, Bot);
  }
  createObject(clusterCount - 1, xleft, xright, ypos, ypos + biny, childOrientation, Top);
}

void RectangleChain::constructHorizontal() {
  //std::cout << "constructHorizontal\n";
  float width = std::fabs(xright - xleft);

  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float binx = width / static_cast<float>(clusterCount - 1);
  float xpos = xleft;

  createObject(0, xleft, xleft + binx, ybot, ytop, childOrientation, Bot);
  for (int i = 0; i < clusterCount - 2; i++) {
    xpos += binx;
    createObject(i + 1, xpos, xpos + binx, ybot, ytop, childOrientation, Bot);
  }
  createObject(clusterCount - 1, xpos, xpos + binx, ybot, ytop, childOrientation, Top);
}

void LineChain::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  //std::cout << "createobject LineChain alignment: " << alignment << "\n";
  objects.push_back(std::make_shared<Line>(Line(botx, topx, boty, topy, widget, objectOrientation, alignment)));
}

void NumberRange::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  objects.push_back(std::make_shared<RectangleText>(
    RectangleText(botx, topx, boty, topy, widget, "Arial", textColor,
      QString::number(from + position * length / (clusterCount - 1), 'f', 1), objectOrientation, alignment)
    ));
}

Gradient::Gradient(float botx, float topx, float boty, float topy, QWidget* widget,
  Orientation orientation, Alignment alignment) :
  Rectangle(botx, topx, boty, topy, widget, orientation, alignment) {

  changeRange = std::min(realHeight, 300.0f);
}

void Gradient::change(Colormap& colormap, const QPoint& newPoint) {
  //std::cout << "realTopy" << realTopy << " realBoty: " << realBoty << " realHeight: " << realHeight << "\n"; 
  //TODO: mby make a single class/method to keep number in range
  if (newPoint.x() > lastChangePoint.x()) {
    contrast = std::max(contrast - std::fabs(lastChangePoint.x() - newPoint.x()) / changeRange / 10.0f, 1.0);
  } else {
    contrast = std::min(contrast + std::fabs(lastChangePoint.x() - newPoint.x()) / changeRange / 10.0f, 100.0);
  }

  //TODO: set limits globaly, join colormap and this
  if (newPoint.y() > lastChangePoint.y()) {
    brightness = std::max(brightness - std::fabs(lastChangePoint.y() - newPoint.y()) / changeRange * 5, -149.0);
  }
  else {
    brightness = std::min(brightness + std::fabs(lastChangePoint.y() - newPoint.y()) / changeRange * 5, 149.0);
  }

  lastChangePoint = newPoint;

  colormap.change(contrast, brightness);

  std::cout << "clickedPointY: " << lastChangePoint.y() << " y: " << newPoint.y() << " contrast: " << contrast << " brightness: " << brightness << "\n";
}

bool Gradient::contains(const QPoint& p) {
  if (p.x() > xleftReal && p.x() < xrightReal && p.y() < ybotReal && p.y() > ytopReal)
    return true;
  //std::cout << "gradient doesnt contain the point " << p.x() << " " << p.y() << " \n";
  //std::cout << "gradient vals x " << realBotx << " " << realTopx << " y " << realBoty << " " << realTopy << "\n";
  return false;
}

void Gradient::clicked(const QPoint& p) {
  isClicked = true;
  lastChangePoint = p;
}

void Gradient::released() {
  isClicked = false;
}