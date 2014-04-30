#ifndef IVIEW_H
#define IVIEW_H

#include "Core.h"
#include "Settings.h"

// base class for tabs in MainWindow

class IView
{
public:

  IView(const QString &connectionId) :
    m_connectionId(connectionId)
  {}

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

  // user showed name
  virtual QString name() const = 0;

  // view id
  virtual QString id() const = 0;

  virtual void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data)
  {
    Q_UNUSED(event);
    Q_UNUSED(data);
  }

  QString connectionId() const
  {
    return m_connectionId;
  }

private:
  QString m_connectionId;
};

#endif // IVIEW_H
