#ifndef GRAPHICSRECTANGLE_H
#define GRAPHICSRECTANGLE_H

#include "colormap.h"

#include <QPainter>
#include <QWidget>
#include <QString>
#include <memory>
#include <cmath>

//TODO: delete later
#include <iostream>

/**
* @brief Provides grahics utility for native opengl painting and functions to construct objects using qpainter.
*/
namespace graphics {

enum Orientation { Horizontal, Vertical };

enum Alignment { None, Center, Bot, Top};

/**
* @brief This is a class used for rendering of rectangle objects.
* Takes coordinates in {-1, 1} range and draws objects scaled according to widgets real proportions.
*/
class Rectangle {
public:
  Rectangle(float botx, float topx, float boty, float topy, QWidget* widget, QColor backgroundColor,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    botx(botx), boty(boty), topx(topx), topy(topy), widget(widget), backgroundColor(backgroundColor),
    orientation(orientation), alignment(alignment) {
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(float botx,float topx, float boty, float topy, QWidget* widget, Orientation orientation = Vertical,
    Alignment alignment = Alignment::None) :
    botx(botx), topx(topx), boty(boty), topy(topy), widget(widget), orientation(orientation), alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  virtual void render();

protected:
  float botx = 0.0f;
  float boty = 0.0f;
  float topx = 0.0f;
  float topy = 0.0f;

  float realBotx = 0.0f;
  float realBoty = 0.0f;
  float realTopx = 0.0f;
  float realTopy = 0.0f;

  float realHeight = 0.0f;
  float realWidth = 0.0f;

  Orientation orientation;
  Alignment alignment;

  //const QWidget& widget;
  QWidget* widget;
  QColor backgroundColor;

  void calculateWidgetProportions();
  virtual void renderTop() { std::cout << "renderTOP\n"; };
  virtual void renderBot() {};
  virtual void renderCenter() {};
  virtual void renderFull();
};

class Line : public Rectangle {
public:
  Line(float botx, float topx, float boty, float topy, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None) :
    Rectangle(botx, topx, boty, topy, widget, orientation, alignment) {
    //std::cout << "Line realBotx: " << realBotx << " realTopx: " << realTopx << " realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

protected:
  void renderTop() override;
  void renderBot() override;
  void renderCenter() override;
};

class RectangleText : public Rectangle {
public:
  RectangleText(float botx, float topx, float boty, float topy, QWidget* widget, QString font,
    QColor textColor, QString text, Orientation orientation = Orientation::Horizontal,
    Alignment alignment = Alignment::None) :
      Rectangle(botx, topx, boty, topy, widget, orientation, alignment), font(font), textColor(textColor),
      text(text) {};

  void setText(const QString& newText) { text = newText; };
protected:
  QString text;
  QString font = "Arial";
  QColor textColor;

  //TODO: refactor text drawing
  void renderFull() override;
  void renderBot() override;
  void renderTop() override;
  void drawText(float x, float y);
};

class RectangleChain : public Rectangle {
public:
  RectangleChain(float bx, float tx, float by, float ty, QWidget* widget, int repeats,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) : 
    Rectangle(bx, tx, by, ty, widget, orientation), clusterCount(repeats), childOrientation(childOrientation) {};

  void render();
  void constructObjects();
protected:
  int clusterCount = 0;
  graphics::Orientation childOrientation;
  std::vector<std::shared_ptr<Rectangle>> objects;
  
  //think this through
  virtual void createObject(int position, float botx, float topx, float boty, float topy, Orientation objectOrientation,
    Alignment alignment) = 0;
  void constructVertical();
  void constructHorizontal();

};

class LineChain : public RectangleChain {
public:
  LineChain(float bx, float tx, float by, float ty, QWidget* widget, int repeats,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) :
    RectangleChain(bx, tx, by, ty, widget, repeats, orientation, childOrientation) {};

protected:
  void createObject(int position, float botx, float topx, float boty, float topy, Orientation objectOrientation,
    Alignment alignment);
};

//TODO: rethink, right now whole t is copied to make
template <class T>
class RectangleChainFactory {
public:
  std::shared_ptr<T> make(T rc) {
    rc.constructObjects();
    return std::make_shared<T>(rc);
  };
};

/**
* @brief This class uses QPaint to render number row.
*/
class NumberRange : public RectangleChain {
  float from = 0.0f;
  float to = 0.0f;
  float length;
  //TODO: make this changable in program options
  QString font = "Arial";
  QColor textColor;

public:
  NumberRange(float bx, float tx, float by, float ty, QWidget* widget, int objectCount,
    float from, float to, QColor color, Orientation orientation = Vertical, Orientation childOrientation = Vertical)
    : RectangleChain(bx, tx, by, ty, widget, objectCount, orientation, childOrientation), from(from), to(to),
      textColor(color), length(std::fabs(to - from)) {};
protected:
  void createObject(int position, float botx, float topx, float boty, float topy, Orientation objectOrientation,
    Alignment alignment);
};

class Gradient : public Rectangle {
public:
  Gradient(float botx, float topx, float boty, float topy, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None) :
    Rectangle(botx, topx, boty, topy, widget, orientation, alignment) {}

  //TODO: mby move this to colormap
  void changeSaturation(Colormap& colormap, float y);
  bool contains(const QPoint& p);

protected:
  float saturation;

};

}
#endif // GRAPHICSRECTANGLE_H
