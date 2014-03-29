#ifndef IVIEW_H
#define IVIEW_H

#include "Settings.h"

// base class for tabs in MainWindow

class IView
{
public:

  IView() {}
  virtual ~IView() {}

  // called after constructor
  virtual void init() = 0;

  // called before destructor
  virtual void tini() = 0;

  // load settings into view
  virtual void restore(Settings &set) = 0;

  // store settings from view
  virtual void store(Settings &set) = 0;

  // widget itself
  virtual QWidget * widget() = 0;

  // name
  virtual QString name() = 0;
};

#endif // IVIEW_H
