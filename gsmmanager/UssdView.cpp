#include "UssdView.h"
#include "ui_UssdView.h"

#include "Core.h"
#include "ModemUssd.h"

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolTip>

#define TABLE_COLUMN_USSD 0
#define TABLE_COLUMN_DESCRIPTION 1

UssdView::UssdView(const QString& connectionId, QWidget *parent) :
  QWidget(parent),
  IView(connectionId),
  ui(new Ui::UssdView)
{
  ui->setupUi(this);

  ui->tableWidget->setColumnCount(2);
  ui->tableWidget->setHorizontalHeaderLabels(QStringList() << tr("USSD") << tr("Description"));
  connect(ui->tableWidget, SIGNAL(ussdUpdated(QString,QString,QString)),
          this, SLOT(updateDbUssd(QString,QString,QString)));
  connect(ui->tableWidget, SIGNAL(ussdDeleted(QList<QString>)),
          this, SLOT(deleteDbUssd(QList<QString>)));
  connect(ui->tableWidget, SIGNAL(ussdActivated(QString)),
          this, SLOT(sendUssd(QString)));

  ui->lineEdit->setValidator(new UssdValidator(this));

  connect(ui->addToPredifinedButton, SIGNAL(clicked()), this, SLOT(addToPredefined()));

  connect(ui->sendUssdButton, SIGNAL(clicked()), this, SLOT(onSendUssdClicked()));
  connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(onSendUssdClicked()));

  connect(ui->pushButtonTerminate, SIGNAL(clicked()), SLOT(terminateSession()));
}

UssdView::~UssdView()
{
  delete ui;
}

void UssdView::changeEvent(QEvent *e)
{
  QWidget::changeEvent(e);
  switch (e->type())
  {
  case QEvent::LanguageChange:
    ui->retranslateUi(this);
    break;
  default:
    break;
  }
}

void UssdView::init()
{
}

void UssdView::tini()
{

}

void UssdView::restore(Settings & /*set*/)
{
  UssdDatabaseEntity ussdDb;
  if (ussdDb.init())
  {
    updateUssd(ussdDb.select());
  }
}

void UssdView::store(Settings & /*set*/)
{

}

QString UssdView::name() const
{
  return tr("USSD");
}

void UssdView::processConnectionEvent(Core::ConnectionEvent event, const QVariant& data)
{
  if (event == Core::ConnectionEventCustom)
  {
    ModemReply* reply = data.value<ModemReply*>();
    Q_ASSERT(reply);

    if ((reply->handlerName() == USSD_HANDLER_NAME))
    {
      const QString errStr = tr("<ERROR>");
      UssdAnswer * ussdAnswer = static_cast<UssdAnswer*> (reply->data());

      switch(reply->type())
      {
      case USSD_REQUEST_SEND:
      {
        if (reply->status())
        {
          receivedUssd(ussdAnswer->ussd, ussdAnswer->status);
        }
        else
        {
          receivedUssd(errStr, USSD_STATUS_DIALOGUE_TERMINATED);
        }
      }
        break;
      default:
        Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown reply type received!");
      }
    }
  }
}

void UssdView::sendUssd(const QString &ussd)
{
  ui->ussdAnswer->appendHtml(QString("<html><head/><body><p><span style=\" "
                                     "color:blue;\"> %1 </span></p></body></html>")
                             .arg(tr("Sent:")));
  ui->ussdAnswer->appendPlainText(ussd);


  ModemRequest * request = Core::instance()->modemRequest(connectionId(), USSD_HANDLER_NAME,
                                                          USSD_REQUEST_SEND, 1);

  UssdArgs * ussdArgs = static_cast<UssdArgs*> (request->args());
  ussdArgs->ussd = ussd;
  ussdArgs->status = USSD_SEND_STATUS_CODE_PRESENTATION_ON;

  Core::instance()->pushRequest(request);

  receivedUssd(QString(), USSD_STATUS_USER_ACTION_NEEDED);
}

void UssdView::receivedUssd(const QString &ussdAnswer, USSD_STATUS status)
{
  if (!ussdAnswer.isEmpty())
  {
    ui->ussdAnswer->appendHtml(QString("<html><head/><body><p><span style=\" "
                                       "color:red;\"> %1 </span></p></body></html>")
                               .arg(tr("Received:")));
    ui->ussdAnswer->appendPlainText(ussdAnswer);
  }

  QString statusStr;

  switch(status)
  {
  case USSD_STATUS_FINISHED:
    statusStr = tr("USSD session: finished.");
    ui->pushButtonTerminate->setEnabled(false);
    break;
  case USSD_STATUS_USER_ACTION_NEEDED:
    statusStr = tr("USSD session: active.");
    ui->pushButtonTerminate->setEnabled(true);
    break;
  case USSD_STATUS_DIALOGUE_TERMINATED:
    statusStr = tr("USSD session: terminated.");
    ui->pushButtonTerminate->setEnabled(false);
    break;
  case USSD_STATUS_OTHER_IO_RESPONDED:
    statusStr = tr("USSD session: other client responded.");
    ui->pushButtonTerminate->setEnabled(false);
    break;
  case USSD_STATUS_OPERATION_NOT_SUPPORTED:
    statusStr = tr("USSD session: operation not supported.");
    ui->pushButtonTerminate->setEnabled(false);
    break;
  case USSD_STATUS_NETWORK_TIMEOUT:
    statusStr = tr("USSD session: network time out.");
    ui->pushButtonTerminate->setEnabled(false);
    break;
  default:
    statusStr = tr("USSD session: unknown.");
    ui->pushButtonTerminate->setEnabled(true);
  }

  ui->labelSessionStatus->setText(statusStr);
}

void UssdView::terminateSession()
{
  ModemRequest * request = Core::instance()->modemRequest(connectionId(), USSD_HANDLER_NAME,
                                                          USSD_REQUEST_SEND, 1);

  UssdArgs * ussdArgs = static_cast<UssdArgs*> (request->args());
  ussdArgs->ussd = QString();
  ussdArgs->status = USSD_SEND_STATUS_DIALOGUE_TERMINATE;

  Core::instance()->pushRequest(request);

  receivedUssd(QString(), USSD_STATUS_DIALOGUE_TERMINATED);
}

void UssdView::updateUssd(const QList<Ussd> &ussdList)
{
  ui->tableWidget->clearContents();
  ui->tableWidget->setRowCount(ussdList.size());

  bool sorted = ui->tableWidget->isSortingEnabled();

  int currentRow = 0;
  foreach(const Ussd &ussd, ussdList)
  {
    QTableWidgetItem *ussdItem = new QTableWidgetItem(ussd.ussd);
    ui->tableWidget->setItem(currentRow, TABLE_COLUMN_USSD, ussdItem);

    QTableWidgetItem *ussdDescrItem = new QTableWidgetItem(ussd.description);
    ui->tableWidget->setItem(currentRow, TABLE_COLUMN_DESCRIPTION, ussdDescrItem);

    ++currentRow;
  }

  ui->tableWidget->setSortingEnabled(sorted);
}

void UssdView::addToPredefined()
{
  QString ussd = ui->lineEdit->text();

  QList<QTableWidgetItem*> items = ui->tableWidget->findItems(ussd, Qt::MatchFixedString);

  if (items.size() == 0)
  {
    // add to table
    {
      bool sorted = ui->tableWidget->isSortingEnabled();
      int row = ui->tableWidget->rowCount();
      ui->tableWidget->insertRow(row);

      QTableWidgetItem *ussdItem = new QTableWidgetItem(ussd);
      ui->tableWidget->setItem(row, TABLE_COLUMN_USSD, ussdItem);

      QTableWidgetItem *ussdDescriptionItem = new QTableWidgetItem(QString());
      ui->tableWidget->setItem(row, TABLE_COLUMN_DESCRIPTION, ussdDescriptionItem);

      ui->tableWidget->setSortingEnabled(sorted);
    }

    // add to database
    {
      UssdDatabaseEntity ussdDb;
      if (ussdDb.init())
      {
        QList<Ussd> ussdList;
        Ussd ussdItem;
        ussdItem.ussd = ussd;
        ussdItem.description = QString();
        ussdList.append(ussdItem);

        ussdDb.insert(ussdList);
      }
    }
  }
  else
  {
    QPoint pos = mapToGlobal(ui->addToPredifinedButton->pos());
    QToolTip::showText(pos, tr("Specified USSD already predefined!"), ui->addToPredifinedButton);
  }
}

void UssdView::updateDbUssd(const QString &oldUssd,
                            const QString &newUssd, const QString &newDescription)
{
  UssdDatabaseEntity ussdDb;
  if (ussdDb.init())
  {
    DatabaseKey key;
    key.insert("a_ussd", oldUssd);

    QList<Ussd> existed = ussdDb.select(key);

    if (existed.size() == 1)
    {
      Ussd ussd = existed.at(0);
      ussd.ussd = newUssd;
      ussd.description = newDescription;
      ussdDb.updat(key, ussd);
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, QString("Selected list size is not equal to 1."));
    }
  }
}

void UssdView::deleteDbUssd(const QList<QString> &ussdList)
{
  UssdDatabaseEntity ussdDb;
  if (ussdDb.init())
  {
    foreach (const QString &ussd, ussdList)
    {
      DatabaseKey key;
      key.insert("a_ussd", ussd);

      ussdDb.delet(key);
    }
  }
}

void UssdView::onSendUssdClicked()
{
  QString ussd = ui->lineEdit->text();

  if(!ussd.isEmpty())
  {
    sendUssd(ussd);
  }
  else
  {
    QPoint pos = mapToGlobal(ui->lineEdit->pos());
    QToolTip::showText(pos, tr("USSD should not be empty!"),
                       ui->lineEdit);
  }

  ui->lineEdit->clear();
}

UssdValidator::UssdValidator(QObject *parent) :
  QValidator(parent)
{
}

void UssdValidator::fixup(QString &input) const
{
  QString fixed;

  QString::iterator iter = input.begin();

  while (iter != input.constEnd())
  {
    QChar c = *iter;

    if ((c == QChar('*')) || (c == QChar('#')) || c.isDigit())
    {
      fixed.append(c);
    }

    ++iter;
  }

  input = fixed;
}

QValidator::State UssdValidator::validate(QString &input, int &pos) const
{
  if (input.isEmpty())
  {
    return QValidator::Intermediate;
  }

  QValidator::State result = QValidator::Acceptable;

  int newPos = 0;
  foreach(const QChar& c, input)
  {
    if ((c != QChar('*')) && (c != QChar('#')) && (!c.isDigit()))
    {
      pos = newPos;
      result = QValidator::Invalid;
      break;
    }

    ++newPos;
  }

  return result;
}


UssdTableWidget::UssdTableWidget(QWidget *parent) :
  QTableWidget(parent)
{
  UssdDelegate * delegate = new UssdDelegate(this);
  setItemDelegate(delegate);
  connect(delegate, SIGNAL(ussdUpdated(QString,QString,QString)),
          this, SIGNAL(ussdUpdated(QString,QString,QString)));

  connect(this, SIGNAL(cellDoubleClicked(int,int)),
          this, SLOT(onCellDoubleClicked(int,int)));
}

void UssdTableWidget::keyPressEvent(QKeyEvent * event)
{
  if (event->matches(QKeySequence::Delete))
  {
    event->accept();
    removeSelectedIndexes();
  }
  else
  {
    QTableWidget::keyPressEvent(event);
  }
}

void UssdTableWidget::contextMenuEvent(QContextMenuEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QTableWidgetItem * item = itemAt(event->pos());

  // construct menu
  QMenu *menu = new QMenu(this);

  QAction editUssd("Edit USSD", this);
  QAction deleteUssd("Delete USSD", this);
  if(!index.isValid())
  {
    editUssd.setEnabled(false);
    deleteUssd.setEnabled(false);
  }

  menu->addAction(&editUssd);
  menu->addAction(&deleteUssd);
  QAction *executed = menu->exec(viewport()->mapToGlobal(event->pos()));

  if (executed == &editUssd)
  {
    editItem(item);
  }
  else if (executed == &deleteUssd)
  {
      removeSelectedIndexes();
  }

  event->accept();
}

void UssdTableWidget::removeSelectedIndexes()
{
  QList<QString> ussdList;

  QModelIndexList selected = selectedIndexes();
  qSort(selected.begin(), selected.end());

  for (int i = selected.size() - 1; i>= 0; --i)
  {
    const QModelIndex &index = selected.at(i);

    // hack: we selecting by complete rows and
    // indexes on the same row but different columns should be ignored
    if (index.isValid() && (index.column() == 0))
    {
      ussdList.append(index.data().toString());
      model()->removeRow(index.row());
    }
  }

  emit ussdDeleted(ussdList);
}

void UssdTableWidget::onCellDoubleClicked(int row, int /*column*/)
{
  QModelIndex index = model()->index(row, TABLE_COLUMN_USSD);

  if(index.isValid())
  {
    QString ussd = index.data().toString();
    emit ussdActivated(ussd);
  }
}


UssdDelegate::UssdDelegate(QObject * parent) :
  QStyledItemDelegate(parent)
{
  m_validator = new UssdValidator(this);
}

QWidget *UssdDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                    const QModelIndex &index) const
{
  QLineEdit * lineEdit = new QLineEdit(parent);

  if (index.column() == TABLE_COLUMN_USSD)
  {
    lineEdit->setValidator(m_validator);
  }

  return lineEdit;
}

void UssdDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
  QLineEdit * lineEdit = qobject_cast<QLineEdit *> (editor);
  Q_ASSERT(lineEdit);

  QString oldUssd;
  QString newUssd;
  QString newDescription;

  if (index.column() == TABLE_COLUMN_USSD)
  {
    oldUssd = index.data().toString();
    newUssd = lineEdit->text();
    newDescription = index.sibling(index.row(), TABLE_COLUMN_DESCRIPTION).data().toString();
  }
  else
  {
    Q_ASSERT(index.column() == TABLE_COLUMN_DESCRIPTION);

    oldUssd = index.sibling(index.row(), TABLE_COLUMN_USSD).data().toString();
    newUssd = oldUssd;
    newDescription = lineEdit->text();
  }

  QStyledItemDelegate::setModelData(editor, model, index);

  emit ussdUpdated(oldUssd, newUssd, newDescription);
}
