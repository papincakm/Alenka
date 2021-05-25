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

class Object {
public:
  Object() {};
  Object(float xleft, float xright, float ybot, float ytop) : xleft(xleft), xright(xright), ybot(ybot), ytop(ytop) {};
  Object(const Object& object) { 
    xleft = object.xleft;
    xright = object.xright;
    ybot = object.ybot;
    ytop = object.ytop;
  };

  float getXleft() { return xleft; };
  float getXright() { return xright; };
  float getYbot() { return ybot; };
  float getYtop() { return ytop; }

protected:
  float xleft = 0.0f;
  float xright = 0.0f;
  float ybot = 0.0f;
  float ytop = 0.0f;
};

class SquareMesh : public Object {
public:
  int rows = 0;
  int columns = 0;

  SquareMesh() {};
  SquareMesh(float xleft, float xright, float ybot, float ytop) : Object(xleft, xright, ybot, ytop) {};
};

/**
* @brief This is a class used for rendering of rectangle objects.
* Takes coordinates in {-1, 1} range and draws objects scaled according to widgets real proportions.
*/
class Rectangle : public Object {
public:
  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget, QColor backgroundColor,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    Object(xleft, xright, ybot, ytop), widget(widget), backgroundColor(backgroundColor),
    orientation(orientation), alignment(alignment) {
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    Object(xleft, xright, ybot, ytop), widget(widget), orientation(orientation), alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(const Object& object, QWidget* widget, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) : Object(object), widget(widget), backgroundColor(backgroundColor),
    orientation(orientation), alignment(alignment) {
    calculateWidgetProportions();
  };

  virtual void render();
  void update();

protected:
  Orientation orientation;
  Alignment alignment;
  QWidget* widget;
  QColor backgroundColor;

  float xleftReal = 0.0f;
  float xrightReal = 0.0f;
  float ybotReal = 0.0f;
  float ytopReal = 0.0f;

  float realHeight = 0.0f;
  float realWidth = 0.0f;

  void calculateWidgetProportions();

  virtual void renderTop() { std::cout << "renderTOP\n"; };
  virtual void renderBot() {};
  virtual void renderCenter() {};
  virtual void renderFull();
};

class Line : public Rectangle {
public:
  Line(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None) :
    Rectangle(xleft, xright, ybot, ytop, widget, orientation, alignment) {
    //std::cout << "Line realBotx: " << realBotx << " realTopx: " << realTopx << " realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

protected:
  void renderTop() override;
  void renderBot() override;
  void renderCenter() override;
};

class RectangleText : public Rectangle {
public:
  RectangleText(float xleft, float xright, float ybot, float ytop, QWidget* widget, QString font,
    QColor textColor, QString text, Orientation orientation = Orientation::Horizontal,
    Alignment alignment = Alignment::None) :
      Rectangle(xleft, xright, ybot, ytop, widget, orientation, alignment), font(font), textColor(textColor),
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
  RectangleChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, int repeats,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) : 
    Rectangle(xleft, xright, ybot, ytop, widget, orientation), clusterCount(repeats), childOrientation(childOrientation) {};

  void render();
  void constructObjects();
protected:
  int clusterCount = 0;
  graphics::Orientation childOrientation;
  std::vector<std::shared_ptr<Rectangle>> objects;
  
  //think this through
  virtual void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
    Alignment alignment) = 0;
  void constructVertical();
  void constructHorizontal();

};

class LineChain : public RectangleChain {
public:
  LineChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, int repeats,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) :
    RectangleChain(xleft, xright, ybot, ytop, widget, repeats, orientation, childOrientation) {};

protected:
  void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
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
  NumberRange(float xleft, float xright, float ybot, float ytop, QWidget* widget, int objectCount,
    float from, float to, QColor color, Orientation orientation = Vertical, Orientation childOrientation = Vertical)
    : RectangleChain(xleft, xright, ybot, ytop, widget, objectCount, orientation, childOrientation), from(from), to(to),
      textColor(color), length(std::fabs(to - from)) {};
protected:
  void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
    Alignment alignment);
};

//TODO: should inherit from class above rectangle, only needs coordinates not rendering, this is rendered later with opengl
class Gradient : public Rectangle {
public:
  Gradient(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None);

  //TODO: mby move this to colormap
  void change(Colormap& colormap, const QPoint& newPoint);
  bool contains(const QPoint& p);
  void clicked(const QPoint& p);
  void released();

  bool isClicked = false;

private:
  float changeRange = 1.0f;
  float contrast = 1.0f;
  float brightness = 0.0f;
  QPoint lastChangePoint;
  
};

}
#endif // GRAPHICSRECTANGLE_H
