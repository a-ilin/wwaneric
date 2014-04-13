#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>

#define SET_MAINWINDOW_GROUP "MainWindow"
#define SET_MAINWINDOW_MINIMIZE_ON_CLOSE "minimizeOnClose"

namespace Ui {
  class AppSettingsDialog;
}

class AppSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AppSettingsDialog(QWidget *parent = 0);
  ~AppSettingsDialog();

public slots:
  void accept();

protected:
  void changeEvent(QEvent *e);

private:
  Ui::AppSettingsDialog *ui;
};

#endif // APPSETTINGSDIALOG_H
