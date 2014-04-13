#include "AppSettingsDialog.h"
#include "ui_AppSettingsDialog.h"

#include "Settings.h"

AppSettingsDialog::AppSettingsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AppSettingsDialog)
{
  ui->setupUi(this);

  Settings set;

  set.beginGroup(SET_MAINWINDOW_GROUP);
  ui->closeButtonLikeMinimize->setChecked(set.value(SET_MAINWINDOW_MINIMIZE_ON_CLOSE).toInt()
                                          ? Qt::Checked : Qt::Unchecked);
  set.endGroup();

}

AppSettingsDialog::~AppSettingsDialog()
{
  delete ui;
}

void AppSettingsDialog::accept()
{
  Settings set;

  set.beginGroup(SET_MAINWINDOW_GROUP);
  set.setValue(SET_MAINWINDOW_MINIMIZE_ON_CLOSE, ui->closeButtonLikeMinimize->isChecked() ? 1 : 0);
  set.endGroup();

  QDialog::accept();
}


void AppSettingsDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  switch (e->type()) {
  case QEvent::LanguageChange:
    ui->retranslateUi(this);
    break;
  default:
    break;
  }
}
