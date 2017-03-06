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

#ifndef MODEMSTATUSVIEW_H
#define MODEMSTATUSVIEW_H

#include "IView.h"

#include <QWidget>

namespace Ui
{
  class ModemStatusView;
}

class QTimer;

class ModemStatusView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit ModemStatusView(const QUuid& connectionId, QWidget *parent = 0);
  ~ModemStatusView();

  void init();
  void tini();

  void restore(Settings &set);
  void store(Settings &set);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "Status";
  }

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

  enum Columns
  {
    ColumnType,
    ColumnInfo,
    ColumnLast
  };

protected:
  void changeEvent(QEvent *e);

  void setStatusInfo(const QString& statusType, const QString& statusInfo);

  void updateSignalQuality(bool noError,
                           bool signalDetected, double signalDbm, double signalPercent,
                           bool berDetected, const QString& berPercentRange);

protected slots:
  void onCopyToClipboard();
  void onTimerTimeout();

private:
  Ui::ModemStatusView *ui;

  // timer for requesting for frequently modificable data
  QTimer * m_timer;

};

#endif // MODEMSTATUSVIEW_H
