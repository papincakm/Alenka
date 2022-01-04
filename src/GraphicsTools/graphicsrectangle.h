#ifndef GRAPHICSRECTANGLE_H
#define GRAPHICSRECTANGLE_H

#include "colormap.h"

#include <QPainter>
#include <QWidget>
#include <QString>
#include <memory>
#include <cmath>
#include <QOpenGLWidget>

/**
* @brief Provides grahics utility for native opengl painting and functions to construct objects using qpainter.
*/
namespace graphics {

enum Orientation { Horizontal, Vertical };

enum Alignment { None, Center, Bot, Top};

/**
* @brief Defines the coordinates of a graphical object.
*/
class GObject {
public:
  GObject() {};
  GObject(float xleft, float xright, float ybot, float ytop) : xleft(xleft), xright(xright), ybot(ybot), ytop(ytop) {};
  GObject(const GObject& object) { 
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

class SquareMesh : public GObject {
public:
  int rows = 0;
  int columns = 0;

  SquareMesh() {};
  SquareMesh(float xleft, float xright, float ybot, float ytop) : GObject(xleft, xright, ybot, ytop) {};
};

/**
* @brief GObject with real Qt coordinates based on the input widget.
*/
class QtObject : public GObject {
public:
  QtObject(float xleft, float xright, float ybot, float ytop, QWidget* widget) :
    GObject(xleft, xright, ybot, ytop), widget(widget) {
    calculateWidgetProportions();
  };

  QtObject(const QtObject& object) : GObject(object) {
    widget = object.widget;
    calculateWidgetProportions();
  };

  QtObject(const GObject& object, QWidget* widget) : GObject(object), widget(widget) {
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
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    QColor backgroundColor, QColor drawingColor, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(xleft, xright, ybot, ytop, widget),
    backgroundColor(backgroundColor), drawingColor(drawingColor), orientation(orientation), alignment(alignment) {
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    QColor backgroundColor, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(boundRect), backgroundColor(backgroundColor),
    orientation(orientation), alignment(alignment) {
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(xleft, xright, ybot, ytop, widget),
    orientation(orientation), alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
  };

  Rectangle(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    Orientation orientation = Orientation::Vertical, Alignment alignment = Alignment::None) :
    QtObject(xleft, xright, ybot, ytop, widget), boundRect(boundRect), orientation(orientation),
    alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
  };

  Rectangle(const GObject& object, QWidget* widget, Orientation orientation = Orientation::Vertical,
    Alignment alignment = Alignment::None) : QtObject(object, widget), boundRect(object, widget),
    orientation(orientation), alignment(alignment) {
    backgroundColor = widget->palette().color(QPalette::Window);
  };

  virtual void render(QPainter* painter);
  void update();

protected:
  QtObject boundRect;
  QColor backgroundColor;
  QColor drawingColor;
  Orientation orientation;
  Alignment alignment;

  virtual void renderTop(QPainter* painter) {};
  virtual void renderBot(QPainter* painter) {};
  virtual void renderCenter(QPainter* painter) {};
  virtual void renderFull(QPainter* painter);
};

class Line : public Rectangle {
public:
  Line(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None) :
    Rectangle(xleft, xright, ybot, ytop, widget, orientation, alignment) {
  };

protected:
  void renderTop(QPainter* painter) override;
  void renderBot(QPainter* painter) override;
  void renderCenter(QPainter* painter) override;
};

class RectangleText : public Rectangle {
public:
  /*RectangleText(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Orientation::Horizontal, Alignment alignment = Alignment::None, QString font = "Arial",
    QColor textColor, QString text, Orientation textOrientation = Orientation::Horizontal) :
      Rectangle(xleft, xright, ybot, ytop, widget, orientation, alignment), font(font), textColor(textColor),
      text(text), textOrientation(textOrientation) {};

  RectangleText(float xleft, float xright, float ybot, float ytop, QWidget* widget, QColor backgroundColor,
    Orientation orientation = Orientation::Horizontal, Alignment alignment = Alignment::None, QString font,
    QColor textColor, Qstring text, Orientation textOrientation = Orientation::Horizontal) :
      Rectangle(xleft, xright, ybot, ytop, widget, backgroundColor, orientation, alignment), font(font), textColor(textColor),
      text(text), textOrientation(textOrientation) {};

  RectangleText(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    Orientation orientation = Orientation::Horizontal, Alignment alignment = Alignment::None,
    QString font, QColor textColor, QString text, Orientation textOrientation = Orientation::Horizontal) :
      Rectangle(xleft, xright, ybot, ytop, boundRect, widget, orientation, alignment), font(font), textColor(textColor),
      text(text), textOrientation(textOrientation) {};

  RectangleText(float xleft, float xright, float ybot, float ytop, const QtObject& boundRect, QWidget* widget,
    QColor backgroundColor, Orientation orientation = Orientation::Horizontal, Alignment alignment = Alignment::None
    OQString font, QColor textColor, QString text, textOrientation = Orientation::Horizontal) :
      Rectangle(xleft, xright, ybot, ytop, boundRect, widget, backgroundColor, orientation, alignment), font(font), textColor(textColor),
      text(text), textOrientation(textOrientation) {
  };*/

  RectangleText(Rectangle rectangle, QString font, QColor textColor, Qstring text, Orientation textOrientation = Orientation::Horizontal) :
    Rectangle(rectangle), font(font), textColor(textColor), text(text), textOrientation(textOrientation) {};

  void setText(const QString& newText) { text = newText; };
protected:
  QString font;
  QColor textColor;
  QString text;
  Orientation textOrientation;

  void renderFull(QPainter* painter) override;
  void renderBot(QPainter* painter) override;
  void renderTop(QPainter* painter) override;
  void drawText(QPainter* painter, float x, float y);
};

/**
* @brief Draws a chain of Rectangle objects.
*/
class RectangleChain : public Rectangle {
public:
  RectangleChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, int objectCount,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) : 
    Rectangle(xleft, xright, ybot, ytop, widget, orientation), objectCount(objectCount), childOrientation(childOrientation) {};

  RectangleChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, QColor backgroundColor, int objectCount,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) :
    Rectangle(xleft, xright, ybot, ytop, widget, backgroundColor, orientation), objectCount(objectCount),
    childOrientation(childOrientation) {};

  void render(QPainter* painter);
  void constructObjects();
protected:
  int objectCount = 0;
  float objectWidth = 0;
  float objectHeight = 0;
  float objectWidthReal = 0;
  float objectHeightReal = 0;

  graphics::Orientation childOrientation;
  std::vector<std::shared_ptr<Rectangle>> objects;
  
  virtual void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
    Alignment alignment) = 0;
  void constructVertical();
  void constructHorizontal();

};

/**
* @brief Draws a chain of Line objects.
*/
class LineChain : public RectangleChain {
public:
  LineChain(float xleft, float xright, float ybot, float ytop, QWidget* widget, int repeats,
    Orientation orientation = Vertical, Orientation childOrientation = Horizontal) :
    RectangleChain(xleft, xright, ybot, ytop, widget, repeats, orientation, childOrientation) {};

protected:
  void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
    Alignment alignment);
};

template <class T>
class RectangleChainFactory {
public:
  std::shared_ptr<T> make(T rc) {
    rc.constructObjects();
    return std::make_shared<T>(rc);
  };
};

/**
* @brief This class uses QPaint to render a number row.
*/
class NumberRange : public RectangleChain {
  float fromNumber = 0.0f;
  float toNumber = 0.0f;
  float precision = 0;
  float length;

  QString font = "Arial";
  QColor textColor;

public:
  NumberRange(float xleft, float xright, float ybot, float ytop, QWidget* widget, int objectCount,
    float from, float to, int precision, QColor textColor, Orientation orientation = Vertical, 
    Orientation childOrientation = Vertical) : RectangleChain(xleft, xright, ybot, ytop, widget, objectCount,
      orientation, childOrientation), fromNumber(from), toNumber(to), precision(precision), textColor(textColor),
      length(std::fabs(to - from)) {};

  NumberRange(float xleft, float xright, float ybot, float ytop, QWidget* widget, int objectCount,
    float from, float to, int precision, QColor backgroundColor, QColor textColor, Orientation orientation = Vertical,
    Orientation childOrientation = Vertical) :
    RectangleChain(xleft, xright, ybot, ytop, widget, backgroundColor, objectCount, orientation,
      childOrientation), fromNumber(from), toNumber(to),
      precision(precision), textColor(textColor), length(std::fabs(to - from)) {};

protected:
  void createObject(int position, float xleft, float xright, float ybot, float ytop, Orientation objectOrientation,
    Alignment alignment);
};

class Gradient : public Rectangle {
public:
  Gradient(float xleft, float xright, float ybot, float ytop, QWidget* widget,
    Orientation orientation = Vertical, Alignment alignment = None);

  Gradient(float xleft, float xright, float ybot, float ytop, QWidget* widget, QColor borderColor,
    QColor backgroundColor, Orientation orientation = Vertical, Alignment alignment = None);

  void change(Colormap& colormap, const QPoint& newPoint);
  bool contains(const QPoint& p);
  void clicked(const QPoint& p);
  void released();
  void generateGradientMesh(std::vector<GLfloat>& triangles, std::vector<GLuint>& indices);
  void reset();

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
