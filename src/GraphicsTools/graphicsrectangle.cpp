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
  painter.drawRect(QRectF(realBotx, realTopy, realWidth, realHeight));
}

void Rectangle::calculateWidgetProportions() {
  //TODO: using brute correction to match opengl drawing
  realBotx = widget->width() / 2.0f + (widget->width() / 2.0f) * botx - 0.5f;
  realTopx = widget->width() / 2.0f + (widget->width() / 2.0f) * topx + 0.5f;
  realBoty = widget->height() / 2.0f + (widget->height() / 2.0f) * boty * -1 + 0.5f;
  realTopy = widget->height() / 2.0f + (widget->height() / 2.0f) * topy * -1 - 0.5f;

  //1.0f is correction to match opengl
  realHeight = abs(realTopy - realBoty);// +1.0f;
  realWidth = abs(realTopx - realBotx);// +1.0f;
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
//TODO: use drawlines ?
void Line::renderTop() {
  //std::cout << " line renderTop\n";
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(realTopx, realTopy, realTopx, realBoty));
  }
  else {
    //painter.drawLine(realBotx, realTopy, realTopx, realTopy);
    painter.drawLine(QLineF(realBotx, realTopy, realTopx, realTopy));
  }
}

void Line::renderBot() {
  //std::cout << " line renderBot\n";
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(realBotx, realTopy, realBotx, realBoty));
  }
  else {
    painter.drawLine(QLineF(realBotx, realBoty, realTopx, realBoty));
  }
}

void Line::renderCenter() {
  QPainter painter(widget);

  if (orientation == Vertical) {
    painter.drawLine(QLineF(realBotx + (realTopx - realBotx) / 2.0f, realBoty, ((realTopx - realBotx) / 2.0f), realTopy));
  }
  else {
    painter.drawLine(QLineF(realBotx, realBoty + (realTopy - realBoty) / 2.0f, realTopx, realBoty + (realTopy - realBoty) / 2.0f));
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
  drawText(realBotx, realTopy);
}

void RectangleText::renderTop() {
  if (orientation == Orientation::Vertical) {
    drawText(realBotx, realTopy);
  }
  else {
    drawText(realTopx, realTopy);
  }

}

void RectangleText::renderBot() {
  if (orientation == Orientation::Vertical) {
    drawText(realBotx, realBoty);
  }
  else {
    drawText(realBotx, realTopy);
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
  float height = std::fabs(topy - boty);
  
  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float biny = height / static_cast<float>(clusterCount - 1);
  float ypos = boty;

  createObject(0, botx, topx, boty, boty + biny, childOrientation, Bot);
  for (int i = 0; i < clusterCount - 2; i++) {
    ypos += biny;
    createObject(i + 1, botx, topx, ypos, ypos + biny, childOrientation, Bot);
  }
  createObject(clusterCount - 1, botx, topx, ypos, ypos + biny, childOrientation, Top);
}

void RectangleChain::constructHorizontal() {
  //std::cout << "constructHorizontal\n";
  float width = std::fabs(topx - botx);

  //std::cout << "topx: " << topx <<  " topy: " << topy  << " height is: " << height << "\n";
  float binx = width / static_cast<float>(clusterCount - 1);
  float xpos = botx;

  createObject(0, botx, botx + binx, boty, topy, childOrientation, Bot);
  for (int i = 0; i < clusterCount - 2; i++) {
    xpos += binx;
    createObject(i + 1, xpos, xpos + binx, boty, topy, childOrientation, Bot);
  }
  createObject(clusterCount - 1, xpos, xpos + binx, boty, topy, childOrientation, Top);
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

bool Gradient::contains(const QPoint& p) {
  if (p.x() > realBotx && p.x() < realTopx && p.y() < realBoty && p.y() > realTopy)
    return true;
  std::cout << "gradient doesnt contain the point " << p.x() << " " << p.y() << " \n";
  std::cout << "gradient vals x " << realBotx << " " << realTopx << " y " << realBoty << " " << realTopy << "\n";
  return false;
}