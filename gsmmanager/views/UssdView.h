#ifndef USSDVIEW_H
#define USSDVIEW_H

#include "../IView.h"
#include "../Ussd.h"

#include <QWidget>

namespace Ui {
  class UssdView;
}

class UssdView : public QWidget, public IView
{
  Q_OBJECT

public:
  explicit UssdView(QWidget *parent = 0);
  ~UssdView();

  void init();
  void tini();

  void restore(Settings &);
  void store(Settings &);

  QWidget * widget() {return this;}

  QString name();

public slots:
  void sendUssd(const QString &ussd);

protected:
  void changeEvent(QEvent *e);

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

protected:
  void keyPressEvent(QKeyEvent * event);
  void contextMenuEvent(QContextMenuEvent * event);

protected slots:
  void removeSelectedIndexes();
  void onCellDoubleClicked(int row, int /*column*/);
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
