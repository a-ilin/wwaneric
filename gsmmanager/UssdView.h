﻿#ifndef USSDVIEW_H
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
  explicit UssdView(const QString &connectionId, QWidget *parent = 0);
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
