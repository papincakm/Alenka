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
  QPainter painter(widget);
  painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.setRenderHints(QPainter::TextAntialiasing);
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
    //painter.drawText(y, x, text);

    //scale
    auto rect = QRectF(y, x, heightReal, textwidth);
    QFontMetrics fm(painter.font());
    qreal sx = rect.width() * 1.0f / fm.width(text);
    qreal sy = rect.height() * 1.0f / fm.height();
    painter.translate(rect.center());
    painter.scale(sx, sy);
    painter.translate(-rect.center());
    painter.drawText(rect, text, Qt::AlignHCenter | Qt::AlignVCenter);
  }
  else {
    //set minimum height
    int textHeight = std::max(heightReal, 20.0f);

    const auto rect = QRectF(x, y, widthReal, textHeight);
    QFontMetrics fm(painter.font());
    qreal sx = rect.width() * 1.0f / fm.width(text);
    qreal sy = rect.height() * 1.0f / fm.height();
    painter.translate(rect.center());
    painter.scale(sx, sy);
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
    //std::cout << "chain item render\n";
    o->render();
  }
}

void RectangleChain::constructVertical() {
  float height = std::fabs(ytop - ybot);
  
  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float biny = height / static_cast<float>(objectCount - 1);
  float ypos = ybot;

  createObject(0, xleft, xright, ypos, ypos + biny, childOrientation, Bot);
  for (int i = 0; i < objectCount - 2; i++) {
    ypos += biny;
    createObject(i + 1, xleft, xright, ypos, ypos + biny, childOrientation, Bot);
  }
  createObject(objectCount - 1, xleft, xright, ypos, ypos + biny, childOrientation, Top);
}

void RectangleChain::constructHorizontal() {
  //std::cout << "constructHorizontal\n";
  float width = std::fabs(xright - xleft);

  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float binx = width / static_cast<float>(objectCount - 1);
  float xpos = xleft;

  createObject(0, xpos, xpos + binx, ybot, ytop, childOrientation, Bot);
  for (int i = 0; i < objectCount - 2; i++) {
    xpos += binx;
    createObject(i + 1, xpos, xpos + binx, ybot, ytop, childOrientation, Bot);
  }
  createObject(objectCount - 1, xpos, xpos + binx, ybot, ytop, childOrientation, Top);
}

void LineChain::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  //std::cout << "createobject LineChain alignment: " << alignment << "\n";
  objects.push_back(std::make_shared<Line>(Line(botx, topx, boty, topy, widget, objectOrientation, alignment)));
}

void NumberRange::createObject(int position, float botx, float topx, float boty, float topy,
  Orientation objectOrientation, Alignment alignment) {
  float xl, xr, yb, yt = 0;
  if (objectOrientation == Orientation::Vertical) {
    xl = botx;
    xr = topx;
    yb = boty + objectHeight / 2;
    yt = yb + objectHeight;
  }
  else {
    xl = botx - objectWidth / 2;
    xr = xl + objectWidth;
    yb = boty;
    yt = topy;
  }

  objects.push_back(std::make_shared<RectangleText>(
    RectangleText(xl, xr, yb, yt, QtObject(botx, topx, boty, topy, widget), widget, "Arial", textColor,
      QString::number(fromNumber + position * length / (objectCount - 1), 'f', 1), objectOrientation,
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

void Gradient::change(Colormap& colormap, const QPoint& newPoint) {
  //std::cout << "realTopy" << realTopy << " realBoty: " << realBoty << " realHeight: " << realHeight << "\n"; 
  //TODO: mby make a single class/method to keep number in range

  float xDist = std::fabs(clickPoint.x() - newPoint.x());
  //dont change contrast if mouse is inside of gradient
  if (xDist > widthReal / 2.0f) {
    if (newPoint.x() > clickPoint.x()) {
      newContrast = std::max(contrast - xDist / changeRangeX, 1.0f);
    }
    else {
      newContrast = std::min(contrast + xDist / changeRangeX, 100.0f);
    }
  }
  //std::cout << "clickPoint x: " << clickPoint.x() << " nePoint x: " << newPoint.x() << "\n";
  //std::cout << "newContrast: " << newContrast << "\n";
  //TODO: set limits globaly, join colormap and this

  float yDist = std::abs(clickPoint.y() - newPoint.y());
  if (newPoint.y() > clickPoint.y()) {
    newBrightness = std::max(brightness - yDist / changeRangeY, -149.0f);
  }
  else {
    newBrightness = std::min(brightness + yDist / changeRangeY, 149.0f);
  }

  colormap.change(newContrast, newBrightness);

  //std::cout << "clickedPointY: " << lastChangePoint.y() << " y: " << newPoint.y() << " contrast: " << contrast << " brightness: " << brightness << "\n";
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
  clickPoint = p;
}

void Gradient::released() {
  isClicked = false;

  brightness = newBrightness;
  contrast = newContrast;
}