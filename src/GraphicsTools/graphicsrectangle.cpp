#include "graphicsrectangle.h"

#include <iostream>

using namespace graphics;

void Rectangle::renderFull() {

  QPainter painter(widget);
  QPen pen(drawingColor);  // creates a default pen

  pen.setWidth(1);
  pen.setJoinStyle(Qt::MiterJoin);

  painter.setPen(pen);

  painter.drawRect(QRectF(xleftReal, ytopReal, widthReal, heightReal));
}

void QtObject::calculateWidgetProportions() {
  //TODO: using brute correction to match opengl drawing
  xleftReal = widget->width() / 2.0f + (widget->width() / 2.0f) * xleft - 0.5f;
  xrightReal = widget->width() / 2.0f + (widget->width() / 2.0f) * xright + 0.5f;
  ybotReal = widget->height() / 2.0f + (widget->height() / 2.0f) * ybot * -1 + 0.5f;
  ytopReal = widget->height() / 2.0f + (widget->height() / 2.0f) * ytop * -1 - 0.5f;

  //1.0f is correction to match opengl
  heightReal = abs(ytopReal - ybotReal);// +1.0f;
  widthReal = abs(xrightReal - xleftReal);// +1.0f;
}

void Rectangle::render() {
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

void Line::renderTop() {
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(xrightReal, ytopReal, xrightReal, ybotReal));
  }
  else {
    painter.drawLine(QLineF(xleftReal, ytopReal, xrightReal, ytopReal));
  }
}

void Line::renderBot() {
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
  QPainter painter(widget);
  painter.setBackgroundMode(Qt::OpaqueMode);
  painter.setBackground(backgroundColor);
  painter.setPen(textColor);
  painter.setBrush(textColor);
  painter.setFont(QFont(font, 8));

  if (textOrientation == Orientation::Vertical) {
    //set minimum height
    int textwidth = std::max(widthReal, 20.0f);

    //rotate
    painter.translate(widget->width() / 2, widget->height() / 2);
    painter.rotate(-90);
    painter.translate(widget->height() / -2, widget->width() / -2);

    //scale
    auto rect = QRectF(y, x, heightReal, textwidth);
    QFontMetrics fm(painter.font());
    qreal sx = rect.width() * 1.0f / fm.width(text);
    painter.translate(rect.center());
    painter.scale(sx, 1);
    painter.translate(-rect.center());
    painter.drawText(rect, text, Qt::AlignHCenter | Qt::AlignVCenter);
  }
  else {
    //set minimum height
    int textHeight = std::max(heightReal, 20.0f);

    const auto rect = QRectF(x, y, widthReal, textHeight);
    QFontMetrics fm(painter.font());
    qreal sy = rect.height() * 1.0f / fm.height();
    painter.translate(rect.center());
    painter.scale(1, sy);
    painter.translate(-rect.center());
    painter.drawText(rect, text, Qt::AlignHCenter | Qt::AlignVCenter);
  }
  painter.end();
}

//same as renderTop rn
void RectangleText::renderFull() {
  if (textOrientation == Orientation::Vertical) {
    drawText(xleftReal, ytopReal);
  }
  else {
    drawText(xleftReal, ytopReal);
  }
}

void RectangleText::renderTop() {
  if (orientation == Orientation::Vertical) {
    drawText(xleftReal, boundRect.getYtopReal() - heightReal);
  }
  else {
    drawText(boundRect.getXrightReal() - widthReal, ytopReal);
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
  objectWidth = std::abs(xleft - xright) / objectCount / 5.0f;
  objectHeight = std::abs(ytop - ybot) / objectCount / 5.0f;
  objectWidthReal = std::abs(xleftReal - xrightReal) / objectCount / 5.0f;
  objectHeightReal = std::abs(ytopReal - ybotReal) / objectCount / 5.0f;

  if (orientation == Vertical) {
    constructVertical();
  }
  else {
    constructHorizontal();
  }
}

void RectangleChain::render() {
  for (auto o : objects) {
    o->render();
  }
}

void RectangleChain::constructVertical() {
  float height = std::fabs(ytop - ybot);
  
  float biny = height / static_cast<float>(objectCount - 1);
  float ypos = ybot;

  for (int i = 0; i < objectCount; i++) {
    createObject(i, xleft, xright, ypos, ypos + biny, childOrientation, Bot);
    ypos += biny;
  }
}

void RectangleChain::constructHorizontal() {
  float width = std::fabs(xright - xleft);

  float binx = width / static_cast<float>(objectCount - 1);
  float xpos = xleft;

  for (int i = 0; i < objectCount; i++) {
    createObject(i, xpos, xpos + binx, ybot, ytop, childOrientation, Bot);
    xpos += binx;
  }
}

void LineChain::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  objects.push_back(std::make_shared<Line>(Line(botx, topx, boty, topy, widget, objectOrientation, alignment)));
}

void NumberRange::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  float xl, xr, yb, yt = 0;
  if (objectOrientation == Orientation::Vertical) {
    xl = botx;
    xr = topx;
    yb = boty;
    yt = yb + objectHeight;
  }
  else {
    xl = botx;
    xr = xl + objectWidth;
    yb = boty;
    yt = topy;
  }

  //precision
  float printNumber = fromNumber + position * length / (objectCount - 1);
  int usedPrecision = precision;
  if (std::floor(printNumber) == printNumber) {
    usedPrecision = 0;
  }

  objects.push_back(std::make_shared<RectangleText>(
    RectangleText(xl, xr, yb, yt, QtObject(botx, topx, boty, topy, widget), widget, "Arial", textColor,
      backgroundColor, QString::number(printNumber, 'f', usedPrecision), objectOrientation,
      Orientation::Horizontal, alignment)
    ));
}

Gradient::Gradient(float botx, float topx, float boty, float topy, QWidget* widget,
  Orientation orientation, Alignment alignment) :
  Rectangle(botx, topx, boty, topy, widget, orientation, alignment) {

  changeRangeY = std::min(heightReal, 300.0f) / 3.0f;
  changeRangeX = std::min(widthReal, 50.0f) * 50;

  newContrast = contrast;
  newBrightness = brightness;
}

Gradient::Gradient(float botx, float topx, float boty, float topy, QWidget* widget, QColor borderColor,
  QColor backgroundColor, Orientation orientation, Alignment alignment) :
  Rectangle(botx, topx, boty, topy, widget, borderColor, backgroundColor, orientation, alignment) {

  changeRangeY = std::min(heightReal, 300.0f) / 3.0f;
  changeRangeX = std::min(widthReal, 50.0f) * 50;

  newContrast = contrast;
  newBrightness = brightness;
}

void Gradient::change(Colormap& colormap, const QPoint& newPoint) {
  float xDist = std::fabs(clickPoint.x() - newPoint.x());
  //dont change contrast if mouse is inside of gradient
  if (xDist > widthReal / 2.0f) {
    if (newPoint.x() > clickPoint.x()) {
      newContrast = std::max(contrast - xDist / changeRangeX, CONTRAST_MIN);
    }
    else {
      newContrast = std::min(contrast + xDist / changeRangeX, CONTRAST_MAX);
    }
  }

  float yDist = std::abs(clickPoint.y() - newPoint.y());
  if (newPoint.y() > clickPoint.y()) {
    newBrightness = std::max(brightness - yDist / changeRangeY, BRIGHTNESS_MIN);
  }
  else {
    newBrightness = std::min(brightness + yDist / changeRangeY, BRIGHTNESS_MAX);
  }

  colormap.change(newContrast, newBrightness);
}

bool Gradient::contains(const QPoint& p) {
  if (p.x() > xleftReal && p.x() < xrightReal && p.y() < ybotReal && p.y() > ytopReal)
    return true;

  return false;
}

void Gradient::clicked(const QPoint& p) {
  isClicked = true;
  clickPoint = p;
}

void Gradient::released() {
  isClicked = false;

  brightness = newBrightness;
  contrast = newContrast;
}

void Gradient::generateGradientMesh(std::vector<GLfloat>& triangles, std::vector<GLuint>& indices) {
  int firstVertex = triangles.size() / 3;

  //1. triangle, left bot vertex
  triangles.push_back(getXleft());
  triangles.push_back(getYbot());
  triangles.push_back(0.01f);
  //TODO: refactor this
  indices.push_back(firstVertex);

  //right bot vertex
  triangles.push_back(getXright());
  triangles.push_back(getYbot());
  triangles.push_back(0.01f);
  indices.push_back(firstVertex + 1);

  //left top vertex
  triangles.push_back(getXleft());
  triangles.push_back(getYtop());
  triangles.push_back(1);
  indices.push_back(firstVertex + 2);

  //2. triangle
  //right bot vertex
  indices.push_back(firstVertex + 1);

  //right top vertex
  triangles.push_back(getXright());
  triangles.push_back(getYtop());
  triangles.push_back(1);
  indices.push_back(firstVertex + 3);

  // left top vertex
  indices.push_back(firstVertex + 2);
}