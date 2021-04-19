#ifndef GRAPHICSRECTANGLE_H
#define GRAPHICSRECTANGLE_H

#include <QPainter>
#include <QWidget>
#include <QString>

namespace graphics {

enum Orientation { Vertical, Horizontal };

/**
* @brief This is an abstract class used for rendering of rectange objects.
* Takes coordinates in {-1, 1} range and draws objects scaled according to widgets real proportions.
*/
class Rectangle {
public:
  Rectangle(float bx, float tx, float by, float ty, QWidget* widget, QColor backgroundColor) :
    botx(bx), boty(by), topx(tx), topy(tx), widget(widget), backgroundColor(backgroundColor) {};

  Rectangle(float bx,float tx, float by, float ty, QWidget* widget) :
    botx(bx), topx(tx), boty(by), topy(ty), widget(widget) {
    backgroundColor = widget->palette().color(QPalette::Window);
  };

  void render();

protected:
  float botx = 0.0f;
  float boty = 0.0f;
  float topx = 0.0f;
  float topy = 0.0f;

  //const QWidget& widget;
  QWidget* widget;
  QColor backgroundColor;
};

class RectangleText : Rectangle {
public:
  RectangleText(float bx, float tx, float by, float ty, QWidget* widget, QString font, QColor textColor, QString text) :
    Rectangle(bx, tx, by, ty, widget), font(font), textColor(textColor), text(text) {};
  
  void render();

protected:
  QString text;
  QString font = "Arial";
  QColor textColor;
};

class RectangleCluster : protected Rectangle {
public:
  RectangleCluster(float bx, float tx, float by, float ty, QWidget* widget, int repeats,
    Orientation orientation = Vertical) : 
    Rectangle(bx, tx, by, ty, widget), clusterCount(repeats), orientation(orientation) {};

  //void render();
protected:
  int clusterCount = 0;
  Orientation orientation;
  
  //think this through
  void renderVertical() {};
  void renderHorizontal() {};
};

/**
* @brief This class uses QPaint to render number row.
*/
class NumberRange : protected RectangleCluster {
  float from = 0.0f;
  float to = 0.0f;
  //TODO: make this changable in program options
  QString font = "Arial";
  QColor textColor;

public:
  NumberRange(float bx, float tx, float by, float ty, QWidget* widget, int repeats,
    float from, float to, QColor color, Orientation orientation = Vertical)
    : RectangleCluster(bx, tx, by, ty, widget, repeats, orientation), from(from), to(to), textColor(color) {};

  void render();
};
}
#endif // GRAPHICSRECTANGLE_H