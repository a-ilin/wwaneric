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

#ifndef USSDVIEW_H
#define USSDVIEW_H

#include "IView.h"
#include "Ussd.h"

#include <QWidget>

namespace Ui {
  class UssdView;
}

class UssdView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit UssdView(const QUuid& connectionId, QWidget *parent = 0);
  ~UssdView();

  void init();
  void tini();

  void restore(Settings &);
  void store(Settings &);

  QWidget * widget() {return this;}

  QString name() const;

  QString id() const
  {
    return "USSD";
  }

  void processConnectionEvent(Core::ConnectionEvent event, const QVariant &data);

public slots:
  void sendUssd(const QString &ussd);
  void terminateSession();

protected:
  void changeEvent(QEvent *e);

  void receivedUssd(const QString &ussdAnswer, USSD_STATUS status);

protected slots:
  void updateUssd(const QList<Ussd> &ussdList);
  void addToPredefined();

  void updateDbUssd(const QString &oldUssd,
                    const QString &newUssd, const QString &newDescription);
  void deleteDbUssd(const QList<QString> &ussdList);

  void onSendUssdClicked();

private:
  Ui::UssdView *ui;
};

#include <QValidator>

class UssdValidator : public QValidator
{
public:
  UssdValidator(QObject * parent = NULL);

  void fixup(QString & input) const;
  State validate(QString & input, int & pos) const;
};


#include <QTableWidget>

class UssdTableWidget : public QTableWidget
{
  Q_OBJECT

public:
  UssdTableWidget(QWidget * parent = NULL);

signals:
  void ussdUpdated(const QString &oldUssd,
                   const QString &newUssd, const QString &newDescription);

  void ussdDeleted(const QList<QString> &ussd);

  void ussdActivated(const QString &ussd);

protected slots:
  void editSelectedIndex();
  void removeSelectedIndexes();
  void onCellDoubleClicked(int row, int /*column*/);
  void onSelectionChanged();
};


#include <QStyledItemDelegate>

class UssdDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  UssdDelegate(QObject *parent = NULL);

signals:
  void ussdUpdated(const QString &oldUssd,
                   const QString &newUssd, const QString &newDescription) const;

protected:
  QWidget *createEditor(QWidget * parent, const QStyleOptionViewItem & /*option*/,
                         const QModelIndex & index) const;

  void setModelData(QWidget * editor, QAbstractItemModel * model,
                    const QModelIndex & index) const;

private:
  UssdValidator * m_validator;

};


#endif // USSDVIEW_H
