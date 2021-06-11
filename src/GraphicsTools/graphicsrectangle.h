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

class QtObject : public Object {
public:
  QtObject(float xleft, float xright, float ybot, float ytop, QWidget* widget) :
    Object(xleft, xright, ybot, ytop), widget(widget) {
    calculateWidgetProportions();
  };

  QtObject(const QtObject& object) {
    xleft = object.xleft;
    xright = object.xright;
    ybot = object.ybot;
    ytop = object.ytop;
    widget = object.widget;
    calculateWidgetProportions();
  };

  QtObject(const Object& object, QWidget* widget) : Object(object), widget(widget) {
    calculateWidgetProportions();
  };

  float getXleftReal() { return xleftReal; };
  float getXrightReal() { return xrightReal; };
  float getYbotReal() { return ybotReal; };
  float getYtopReal() { return ytopReal; }

protected:
  QWidget* widget;

  float xleftReal = 0.0f;
  float xrightReal = 0.0f;
  float ybotReal = 0.0f;
  float ytopReal = 0.0f;

  float heightReal = 0.0f;
  float widthReal = 0.0f;

  void calculateWidgetProportions();
};

/**
* @brief This is a class used for rendering of rectangle objects.
* Takes coordinates in {-1, 1} range and draws objects scaled according to widgets real proportions.
*/
class Rectangle : public QtObject {
public:
  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget, QColor backgroundColor,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(xleft, xright, ybot, ytop, widget),
    backgroundColor(backgroundColor), orientation(orientation), alignment(alignment) {
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    QColor backgroundColor, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(boundRect), backgroundColor(backgroundColor),
    orientation(orientation), alignment(alignment) {
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(xleft, xright, ybot, ytop, widget),
    orientation(orientation), alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(boundRect), orientation(orientation),
    alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
    calculateWidgetProportions();
    //std::cout << "Rectangle realBoty: " << realBoty << " realTopy: " << realTopy << "\n";
  };

  Rectangle(const Object& object, QWidget* widget, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) : QtObject(object, widget), boundRect(object, widget),
    backgroundColor(backgroundColor), orientation(orientation), alignment(alignment) {
    calculateWidgetProportions();
  };

  virtual void render();
  void update();

protected:
  Orientation orientation;
  Alignment alignment;
  QColor backgroundColor;
  QtObject boundRect;

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
    Orientation textOrientation = Orientation::Horizontal, Alignment alignment = Alignment::None) :
      Rectangle(xleft, xright, ybot, ytop, widget, orientation, alignment), font(font), textColor(textColor),
      text(text), textOrientation(textOrientation) {};

  RectangleText(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    QString font, QColor textColor, QString text, Orientation orientation = Orientation::Horizontal,
    Orientation textOrientation = Orientation::Horizontal, Alignment alignment = Alignment::None) :
    Rectangle(xleft, xright, ybot, ytop, boundRect, widget, orientation, alignment), font(font), textColor(textColor),
    text(text), textOrientation(textOrientation) {};

  void setText(const QString& newText) { text = newText; };
protected:
  QString text;
  QString font = "Arial";
  QColor textColor;
  Orientation textOrientation;

  //TODO: refactor text drawing
  void renderFull() override;
  void renderBot() override;
  void renderTop() override;
  void drawText(float x, float y);
};

class RectangleChain : public Rectangle {
public:
  RectangleChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, int objectCount,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) : 
    Rectangle(xleft, xright, ybot, ytop, widget, orientation), objectCount(objectCount), childOrientation(childOrientation) {};

  void render();
  void constructObjects();
protected:
  int objectCount = 0;
  //TODO: mby use better system for converting (scale or smthng), not storing stuff everywhere
  float objectWidth = 0;
  float objectHeight = 0;
  float objectWidthReal = 0;
  float objectHeightReal = 0;

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
  float changeRangeY = 1.0f;
  float changeRangeX = 1.0f;
  float contrast = 1.0f;
  float brightness = 0.0f;

  float newContrast;
  float newBrightness;

  QPoint clickPoint;
  
};

}
#endif // GRAPHICSRECTANGLE_H
