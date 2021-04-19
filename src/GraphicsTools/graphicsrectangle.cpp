#include "graphicsrectangle.h"

#include <iostream>

using namespace graphics;

void Rectangle::render() {
  qreal realBotx = widget->width() / 2.0f + (widget->width() / 2.0f) * botx;
  qreal realTopx = widget->width() / 2.0f + (widget->width() / 2.0f) * topx;
  qreal realBoty = widget->height() / 2.0f + (widget->height() / 2.0f) * boty * -1;
  qreal realTopy = widget->height() / 2.0f + (widget->height() / 2.0f) * topy * -1;

  qreal height = abs(realTopy - realBoty);
  qreal width = abs(realTopx - realBotx);

  QPainter painter(widget);
  QPen pen;

  pen.setColor(Qt::black);
  pen.setWidth(5);

  painter.setPen(pen);
  painter.drawRect(QRect(realBotx, realTopy, width, height));
}

void RectangleText::render() {
  qreal realBotx = widget->width() / 2.0f + (widget->width() / 2.0f) * botx;
  qreal realTopx = widget->width() / 2.0f + (widget->width() / 2.0f) * topx;
  qreal realBoty = widget->height() / 2.0f + (widget->height() / 2.0f) * boty * -1;
  qreal realTopy = widget->height() / 2.0f + (widget->height() / 2.0f) * topy * -1;

  qreal height = abs(realTopy - realBoty);
  qreal width = abs(realTopx - realBotx);

  int fontSize = std::max(8, static_cast<int>(height / 5.0f));
  std::cout << "text rendering text on y: " << realBoty << " boty is " << boty << " fontsize is : " << fontSize << "\n";
  //text is corrupted if this is not called
  //gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  QPainter painter(widget);
  painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.setRenderHints(QPainter::TextAntialiasing);
  painter.setBackground(backgroundColor);
  painter.setPen(textColor);
  painter.setBrush(textColor);
  painter.setFont(QFont(font, fontSize, QFont::Bold));
  painter.drawText(QRectF(realBotx, realTopy, width, height), text);
  painter.end();
}

void NumberRange::render() {
  if (orientation == Vertical) {
    //QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);
    float maxMinusMinX = topx - botx;
    float maxMinusMinY = topy - boty;
    float maxMinusMinNum = to - from;

    float binx = maxMinusMinX / static_cast<float>(clusterCount - 1);
    float biny = maxMinusMinY / static_cast<float>(clusterCount);
    float binNum = maxMinusMinNum / static_cast<float>(clusterCount - 1);

    float numToDisplay = from;
    float xpos = botx;
    float ypos = boty;

    for (int i = 0; i < clusterCount; i++) {
      //TODO: tie this to font height somehow
      //if (i == (clusterCount - 1))
        //xpos -= 0.065f;

      std::cout << "rendering text on y: " << ypos << "\n";
      auto text = RectangleText(botx, topx, ypos, ypos + biny / 2.0f, widget, "Arial", textColor, QString::number(numToDisplay, 'f', 1));
      text.render();

      numToDisplay += binNum;
      //xpos += binx;
      ypos += biny;
    }
  }
  else {
    //QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);
    float maxMinusMinX = topx - botx;
    float maxMinusMinY = topy - boty;
    float maxMinusMinNum = to - from;

    float binx = maxMinusMinX / static_cast<float>(clusterCount);
    float biny = maxMinusMinY / static_cast<float>(clusterCount);
    float binNum = maxMinusMinNum / static_cast<float>(clusterCount - 1);

    float numToDisplay = from;
    float xpos = botx;
    float ypos = boty;

    for (int i = 0; i < clusterCount; i++) {
      //TODO: tie this to font height somehow
      //if (i == (clusterCount - 1))
      //xpos -= 0.065f;

      std::cout << "rendering text on x: " << xpos << "\n";
      auto text = RectangleText(xpos, xpos + binx, boty, topy, widget, "Arial", textColor, QString::number(numToDisplay, 'f', 1));
      text.render();

      numToDisplay += binNum;
      //xpos += binx;
      xpos += binx;
    }
  }
}

/*void GraphicsHorizontalNumberRange::render() {
  QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);

  float maxMinusMinY = range.topY - range.botY;
  float maxMinusMinNum = range.to - range.from;

  float binY = maxMinusMinY / (range.numberCount - 1);
  float binNum = maxMinusMinNum / (range.numberCount - 1);

  float numToDisplay = range.from;
  float yPos = range.botY;

  for (int i = 0; i < range.numberCount; i++) {
    //TODO: tie this to font height somehow
    if (i == (range.numberCount - 1))
      yPos -= 0.065f;

    renderText(range.botX, yPos, QString::number(numToDisplay, 'f', 3), gradientNumberFont, color);
    numToDisplay += binNum;
    yPos += binY;
  }
}

void TfVisualizer::renderHorizontal(const GraphicsNumberRange& range, QColor color) {
  QFont gradientNumberFont = QFont(range.font, 13, QFont::Bold);

  float maxMinusMinX = range.topX - range.botX;
  float maxMinusMinNum = range.to - range.from;

  float binX = maxMinusMinX / (range.numberCount - 1);
  float binNum = maxMinusMinNum / (range.numberCount - 1);

  float numToDisplay = range.from;
  float xPos = range.botX;

  for (int i = 0; i < range.numberCount; i++) {
    //TODO: tie this to font height somehow
    if (i == (range.numberCount - 1))
      xPos -= 0.065f;

    renderText(xPos, range.botY, QString::number(numToDisplay, 'f', 1), gradientNumberFont, color);
    numToDisplay += binNum;
    xPos += binX;
  }
}

void TfVisualizer::renderText(float x, float y, const QString& str, const QFont& font, const QColor& fontColor) {
  int realX = width() / 2 + (width() / 2) * x;
  int realY = height() / 2 + (height() / 2) * y * -1;

  //text is corrupted if this is not called
  //gl()->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  QPainter painter(this);
  painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.setRenderHints(QPainter::TextAntialiasing);
  painter.setBackground(QBrush(QColor(0, 0, 0)));
  painter.setPen(fontColor);
  painter.setBrush(fontColor);
  painter.setFont(font);
  painter.drawText(realX, realY, str);
  painter.end();
}*/