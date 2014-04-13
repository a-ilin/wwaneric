#ifndef DEBUGPARSERS_H
#define DEBUGPARSERS_H

#include <QDialog>

namespace Ui {
  class DebugParsers;
}

class DebugParsers : public QDialog
{
  Q_OBJECT

public:
  explicit DebugParsers(QWidget *parent = 0);
  ~DebugParsers();

protected:
  void changeEvent(QEvent *e);

protected slots:
  void SBA_split();
  void PAL_split();

private:
  Ui::DebugParsers *ui;
};

#endif // DEBUGPARSERS_H
