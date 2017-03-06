/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef IVIEW_H
#define IVIEW_H

#include "Core.h"
#include "Settings.h"

// base class for tabs in MainWindow

class IView
{
public:

  IView(const QUuid& connectionId) :
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

  // user displayed name
  virtual QString name() const = 0;

  // view id
  virtual QString id() const = 0;

  virtual void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data)
  {
    Q_UNUSED(event);
    Q_UNUSED(data);
  }

  QUuid connectionId() const
  {
    return m_connectionId;
  }

private:
  QUuid m_connectionId;
};

#endif // IVIEW_H
