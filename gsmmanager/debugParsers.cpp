#include "DebugParsers.h"
#include "ui_DebugParsers.h"

#include "common.h"
#include "Modem.h"

#include <QListWidgetItem>
#include <QTimer>

DebugParsers::DebugParsers(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DebugParsers)
{
  ui->setupUi(this);

  // Split QByteArray
  ui->SBA_mode->addItem("Keep data only", KeepDataOnly);
  ui->SBA_mode->addItem("Keep empty parts", KeepEmptyParts);
  ui->SBA_mode->addItem("Keep separators", KeepSeparators);
  connect(ui->SBA_mode, SIGNAL(currentIndexChanged(int)), SLOT(SBA_split()));
  connect(ui->SBA_source, SIGNAL(textChanged(QString)), SLOT(SBA_split()));
  connect(ui->SBA_separator, SIGNAL(textChanged(QString)), SLOT(SBA_split()));
  connect(ui->SBA_split, SIGNAL(clicked()), SLOT(SBA_split()));
  QTimer::singleShot(500, this, SLOT(SBA_split()));

  // Parse answer line
  connect(ui->PAL_source, SIGNAL(textChanged(QString)), SLOT(PAL_split()));
  connect(ui->PAL_command, SIGNAL(textChanged(QString)), SLOT(PAL_split()));
  connect(ui->PAL_split, SIGNAL(clicked()), SLOT(PAL_split()));
  QTimer::singleShot(500, this, SLOT(PAL_split()));
}

DebugParsers::~DebugParsers()
{
  delete ui;
}

void DebugParsers::changeEvent(QEvent *e)
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

void DebugParsers::SBA_split()
{
  SplitByteArrayMode mode = (SplitByteArrayMode)ui->SBA_mode->currentData().toInt();

  QList<QByteArray> splitted = splitByteArray(ui->SBA_source->text().toUtf8(), ui->SBA_separator->text().toUtf8(), mode);

  ui->SBA_result->clear();

  int count = 0;
  foreach(const QByteArray &array, splitted)
  {
    ui->SBA_result->addItem(QString(array));
    ui->SBA_result->item(count)->setBackgroundColor(Qt::cyan);
    ++count;
  }
}

void DebugParsers::PAL_split()
{
  QStringList splitted = parseAnswerLine(ui->PAL_source->text(), ui->PAL_command->text());
  ui->PAL_result->clear();

  int count = 0;
  foreach(const QString &str, splitted)
  {
    ui->PAL_result->addItem(str);
    ui->PAL_result->item(count)->setBackgroundColor(Qt::cyan);
    ++count;
  }
}
