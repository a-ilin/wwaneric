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

#ifndef SERIALPORTSETTINGSVIEW_H
#define SERIALPORTSETTINGSVIEW_H

#include "IView.h"

#include <QWidget>

namespace Ui
{
  class SerialPortSettingsView;
}

class SerialPortSettingsView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit SerialPortSettingsView(const QUuid& connectionId, QWidget *parent = 0);
  ~SerialPortSettingsView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "SerialPortSettings";
  }

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

public slots:


signals:

protected:
  void changeEvent(QEvent *e);

private:


private slots:
  void openPortClicked();
  void closePortClicked();

  void defaultCheckBoxStateChanged(int state);

private:
  Ui::SerialPortSettingsView *ui;

};

#endif // SERIALPORTSETTINGSVIEW_H
