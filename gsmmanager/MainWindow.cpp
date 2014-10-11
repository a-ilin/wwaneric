#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Modem.h"

#include "IView.h"
#include "ModemStatusView.h"
#include "SmsView.h"
#include "UssdView.h"
#include "SerialPortSettingsView.h"

#include "ModemStatus.h"

#include "AppSettingsDialog.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QDockWidget>
#include <QInputDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QUrl>
#include <QUuid>

#ifdef QT_DEBUG
#include "debugParsers.h"
#endif

#define SET_GROUP_PREFIX "grp_"
#define SET_VIEW_PREFIX  "view_"
#define SET_GROUP_NAME "groupName"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_exit(true),
  m_minimizeOnClose(false)
{
  ui->setupUi(this);

  // tray icon
  QAction * showAction = new QAction(tr("Show"), this);
  connect(showAction, SIGNAL(triggered()), SLOT(onShowAction()));

  m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
  m_trayIcon->show();
  connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

  m_trayMenu = new QMenu(this);
  m_trayMenu->addAction(showAction);
  m_trayMenu->addSeparator();
  m_trayMenu->addAction(ui->actionExit);
  m_trayMenu->setDefaultAction(showAction);

  m_trayIcon->setContextMenu(m_trayMenu);

  // main menu actions
  connect(ui->actionExit, SIGNAL(triggered()), SLOT(onExitAction()));
  connect(ui->actionAdd_connection, SIGNAL(triggered()), SLOT(addConnection()));
  connect(ui->actionPreferences, SIGNAL(triggered()), SLOT(showPreferences()));
  connect(ui->actionVisit_website, SIGNAL(triggered()), SLOT(visitWebsite()));
  connect(ui->actionAbout, SIGNAL(triggered()), SLOT(showAbout()));
  connect(ui->actionAbout_Qt, SIGNAL(triggered()), SLOT(showAboutQt()));

  // UI
  setTabShape(QTabWidget::Triangular);
  setDockNestingEnabled(true);

  // connections widget
  ui->connectionsWidget->setColumnCount(ColumnLast);

  // Core connections
  connect(Core::instance(), SIGNAL(connectionEvent(QUuid,Core::ConnectionEvent,QVariant)),
          this, SLOT(onConnectionEvent(QUuid,Core::ConnectionEvent,QVariant)),
          Qt::DirectConnection);

  // application style sheet
  QString appStyleSheet =
      /* UI background color */
      "QComboBox, QLineEdit, QListView, QPlainTextEdit, QTableView"
      "{"
      "background-color: rgb(255, 255, 240);"
      "}"

      /* QDockWidget title, handle, buttons */
      "QDockWidget "
      "{"
      "  font: bold;"
      "}"
      "QDockWidget::title "
      "{"
      "  font: bold;"
      "  background: DarkSeaGreen;"
      "  padding-left: 10px;"
      "  padding-top: 4px;"
      "}"
      "QDockWidget::close-button "
      "{"
      "   background: DarkSeaGreen;"
      "}"
      "QDockWidget::float-button "
      "{"
      "   background: DarkSeaGreen;"
      "}"
      ;

  qApp->setStyleSheet(appStyleSheet);

  // debug windows
#ifdef QT_DEBUG
  DebugParsers * dbgParsers = new DebugParsers(this);

  QMenu * debugMenu = ui->menuBar->addMenu("Debug");
  debugMenu->addAction("Debug parsers", dbgParsers, SLOT(show()));
#endif
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
  QMainWindow::changeEvent(e);
  switch (e->type())
  {
  case QEvent::LanguageChange:
    ui->retranslateUi(this);
    break;
  default:
    break;
  }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (!m_exit)
  {
    showMinimized();
    hide();

    m_trayIcon->showMessage(tr("wwanEric is still running here."),
                            tr("Notifications will be showed when something important occures."));

    event->ignore();
  }
}

QList<IView *> MainWindow::createViews(const QUuid& uuid)
{
  QList<IView *> views;

  views.append(new ModemStatusView(uuid, this));
  views.append(new SmsView(uuid, this));
  views.append(new UssdView(uuid, this));
  views.append(new SerialPortSettingsView(uuid, this));

  Settings set;
  set.beginGroup(QString(SET_GROUP_PREFIX) + uuid.toString());

  foreach(IView* view, views)
  {
    view->init();

    set.beginGroup(QString(SET_VIEW_PREFIX) + view->id());
    view->restore(set);
    set.endGroup();
  }

  set.endGroup();

  return views;
}

void MainWindow::addViewGroup(const QUuid& uuid)
{
  // create new one
  if(! m_connBoxes.contains(uuid))
  {
    // core connection
    Core::instance()->createConnection(uuid);

    ConnBox& cBoxes = m_connBoxes[uuid];

    // group menu
    QMenu * menuGroup = new QMenu(uuid.toString(), this);
    ui->menuConnections->addMenu(menuGroup);
    menuGroup->setObjectName(uuid.toString());

    QList<QDockWidget*> dockWidgets;

    // views
    QList<IView *> views = createViews(uuid);
    foreach(IView* view, views)
    {
      // dock widget create
      QDockWidget * dockWidget = new QDockWidget(uuid.toString(), this);
      dockWidget->setObjectName(view->id());
      dockWidget->setWidget(view->widget());
      dockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
      addDockWidget(Qt::RightDockWidgetArea, dockWidget);
      dockWidgets.append(dockWidget);

      // index
      Box box;
      box.view = view;
      box.container = dockWidget;
      cBoxes.boxes.append(box);
    }

    // tabify created dock widgets
    for (int i=0; i<dockWidgets.size()-1 ; ++i)
    {
      tabifyDockWidget(dockWidgets.at(i), dockWidgets.at(i+1));
    }
    if (dockWidgets.size())
    {
      dockWidgets.at(0)->raise();
    }

    // group menu
    menuGroup->addSeparator();
    QAction * renameAction = menuGroup->addAction(tr("Rename connection"));
    renameAction->setData(uuid);
    renameAction->setIcon(QIcon("icons/application_rename_connection.png"));
    connect(renameAction, SIGNAL(triggered()), SLOT(onRenameGroupAction()));

    QAction * removeAction = menuGroup->addAction(tr("Remove connection"));
    removeAction->setData(uuid);
    removeAction->setIcon(QIcon("icons/application_delete_connection.png"));
    connect(removeAction, SIGNAL(triggered()), SLOT(onRemoveGroupAction()));

    // connection widget item
    {
      int row = ui->connectionsWidget->rowCount();
      ui->connectionsWidget->setRowCount(row + 1);

      QTableWidgetItem * itemName = new QTableWidgetItem(uuid.toString());
      itemName->setData(Qt::UserRole, uuid);
      ui->connectionsWidget->setItem(row, ColumnName, itemName);

      QTableWidgetItem * itemStatus = new QTableWidgetItem(QString());
      ui->connectionsWidget->setItem(row, ColumnStatus, itemStatus);

      QTableWidgetItem * itemSignal = new QTableWidgetItem(QString());
      ui->connectionsWidget->setItem(row, ColumnSignalStrength, itemSignal);

      updateConnectionStatus(uuid, false);
      updateSignalStrength(uuid, 0.0);
    }
  }
  else
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("UUID is not unique: ") + uuid.toString());
  }
}

void MainWindow::restore()
{
  Settings set;
  set.beginGroup(SET_MAINWINDOW_GROUP);
  m_minimizeOnClose = set.value(SET_MAINWINDOW_MINIMIZE_ON_CLOSE).toInt();
  m_exit = !m_minimizeOnClose;
  QStringList groups = set.value("groups").toStringList();
  set.endGroup(); // SET_MAINWINDOW_GROUP

  foreach (const QString &uuidString, groups)
  {
    QUuid uuid(uuidString);

    addViewGroup(uuid);

    // group name
    Settings set;
    set.beginGroup(QString(SET_GROUP_PREFIX) + uuid.toString());
    QString groupName = set.value(SET_GROUP_NAME, uuid.toString()).toString();
    set.endGroup();
    setViewGroupName(uuid, groupName);
  }

  // geometry, state
  set.beginGroup(SET_MAINWINDOW_GROUP);
  restoreGeometry(set.value("windowGeometry").toByteArray());
  restoreState(set.value("windowState").toByteArray());
  set.endGroup(); // SET_MAINWINDOW_GROUP
}

void MainWindow::store()
{
  Settings set;

  set.beginGroup(SET_MAINWINDOW_GROUP);
  set.setValue(SET_MAINWINDOW_MINIMIZE_ON_CLOSE, m_minimizeOnClose ? 1 : 0);

  QList<QUuid> uuids = m_connBoxes.uniqueKeys();
  QStringList uuidStrings;
  uuidStrings.reserve(uuids.size());
  foreach (const QUuid& uuid, uuids)
  {
    uuidStrings.append(uuid.toString());
  }

  set.setValue("groups", uuidStrings);

  // geometry, state
  set.setValue("windowGeometry", saveGeometry());
  set.setValue("windowState", saveState());

  set.endGroup(); // SET_MAINWINDOW_GROUP
}

int MainWindow::connectionWidgetRow(const QUuid& uuid) const
{
  QAbstractItemModel * m = ui->connectionsWidget->model();
  Q_ASSERT(m->rowCount() > 0);
  QModelIndexList indexes = m->match(m->index(0, ColumnName), Qt::UserRole, uuid);
  Q_ASSERT(indexes.size() == 1);

  int row = indexes.first().row();
  return row;
}

QString MainWindow::askUserConnectionName(const QUuid& uuid)
{
  bool ok = false;
  QString defaultName = m_connBoxes.value(uuid).name;
  QString name = QInputDialog::getText(this, tr("Specify connection name"),
                                       tr("Name: "), QLineEdit::Normal, defaultName, &ok);

  QString errorTitle = tr("Connection creation error");

  if (! ok)
  {
    return QString();
  }

  if (name.isEmpty())
  {
    QMessageBox::critical(this, errorTitle, tr("Connection name cannot be empty!"));
    return QString();
  }

  // check uniqueness
  bool unique = true;

  ConnectMap::const_iterator iter = m_connBoxes.constBegin();
  ConnectMap::const_iterator iterEnd = m_connBoxes.constEnd();
  while (iter != iterEnd)
  {
    if ( (iter.key() != uuid) && ((*iter).name == name) )
    {
      unique = false;
      break;
    }

    ++iter;
  }

  if (! unique)
  {
    QMessageBox::critical(this, errorTitle, tr("Connection name should be unique!"));
    return QString();
  }

  return name;
}

void MainWindow::updateConnectionStatus(const QUuid& connectionId, bool status)
{
  int row = connectionWidgetRow(connectionId);

  QTableWidgetItem * statusItem = ui->connectionsWidget->item(row, ColumnStatus);
  Q_ASSERT(statusItem);

  if (status)
  {
    statusItem->setIcon(QIcon("icons/port_opened.png"));
    statusItem->setToolTip(tr("Port opened"));
  }
  else
  {
    statusItem->setIcon(QIcon("icons/port_closed.png"));
    statusItem->setToolTip(tr("Port closed"));
  }

  ui->connectionsWidget->resizeColumnsToContents();
}

void MainWindow::updateSignalStrength(const QUuid& connectionId, double strengthPercent)
{
  QIcon signalIcon;

  if (strengthPercent <= 0.1)
  {
    signalIcon = QIcon("icons/signal_strength_000.png");
  }
  else if (strengthPercent <= 20)
  {
    signalIcon = QIcon("icons/signal_strength_020.png");
  }
  else if (strengthPercent <= 40)
  {
    signalIcon = QIcon("icons/signal_strength_040.png");
  }
  else if (strengthPercent <= 60)
  {
    signalIcon = QIcon("icons/signal_strength_060.png");
  }
  else if (strengthPercent <= 80)
  {
    signalIcon = QIcon("icons/signal_strength_080.png");
  }
  else
  {
    signalIcon = QIcon("icons/signal_strength_100.png");
  }

  int row = connectionWidgetRow(connectionId);

  QTableWidgetItem * signalItem = ui->connectionsWidget->item(row, ColumnSignalStrength);
  Q_ASSERT(signalItem);

  signalItem->setIcon(signalIcon);
  signalItem->setToolTip(tr("Signal strength: %1%").arg((int)strengthPercent));

  ui->connectionsWidget->resizeColumnsToContents();
}

void MainWindow::removeViewGroup(const QUuid& uuid, bool storeSettings)
{
  // FIXME: remove archived SMS for group!

  ConnectMap::iterator iter = m_connBoxes.find(uuid);

  if (iter != m_connBoxes.end())
  {
    const ConnBox& cBox = m_connBoxes[uuid];
    const QList<Box>& boxes = cBox.boxes;

    // remove dock widgets from MainWindow
    foreach(const Box& box, boxes)
    {
      removeDockWidget(box.container);
    }

    // store views
    if (storeSettings)
    {
      Settings set;
      set.beginGroup(QString(SET_GROUP_PREFIX) + uuid.toString());
      set.setValue(SET_GROUP_NAME, cBox.name);

      foreach(const Box &box, boxes)
      {
        IView * view = box.view;

        set.beginGroup(QString(SET_VIEW_PREFIX) + view->id());
        view->store(set);
        set.endGroup();
      }

      set.endGroup();
    }
    else
    {
      Settings set;
      set.remove(QString(SET_GROUP_PREFIX) + uuid.toString());
    }

    // delete views
    foreach(const Box &box, boxes)
    {
      IView * view = box.view;
      view->tini();
      delete view;
    }

    // delete dock widgets
    foreach(const Box& box, boxes)
    {
      box.container->deleteLater();
    }

    // menu
    QMenu * menu = findChild<QMenu*>(uuid.toString());
    Q_ASSERT(menu);
    if (menu)
    {
      menu->clear();
      ui->menuConnections->removeAction(menu->menuAction());
      menu->deleteLater();
    }

    // core connection
    Core::instance()->removeConnection(uuid);

    // connections widget
    {
      int row = connectionWidgetRow(uuid);
      ui->connectionsWidget->removeRow(row);
    }

    m_connBoxes.erase(iter);
  }
}

void MainWindow::setViewGroupName(const QUuid& uuid, const QString& userName)
{
  // container
  ConnectMap::iterator iter = m_connBoxes.find(uuid);
  Q_ASSERT(iter != m_connBoxes.end());
  ConnBox& cBox = *iter;
  const QList<Box>& boxes = cBox.boxes;
  cBox.name = userName;

  // dock widgets
  foreach (const Box& box, boxes)
  {
    QString name = QString("%1 - %2").arg(userName).arg(box.view->name());
    box.container->setWindowTitle(name);
  }

  // connection widget
  int row = connectionWidgetRow(uuid);
  QTableWidgetItem * nameItem = ui->connectionsWidget->item(row, ColumnName);
  nameItem->setText(userName);

  // menu
  QMenu * menu = findChild<QMenu*>(uuid.toString());
  Q_ASSERT(menu);
  menu->menuAction()->setText(userName);
}

void MainWindow::init()
{
  restore();
}

void MainWindow::tini()
{
  store();

  QList<QUuid> uuids = m_connBoxes.uniqueKeys();
  foreach(const QUuid& uuid, uuids)
  {
    removeViewGroup(uuid, true);
  }
}

void MainWindow::addConnection()
{
  QUuid uuid = QUuid::createUuid();
  QString name = askUserConnectionName(uuid);
  if (! name.isEmpty())
  {
    addViewGroup(uuid);
    setViewGroupName(uuid, name);
  }
}

void MainWindow::showPreferences()
{
  Core::instance()->storeSettings();
  store();

  AppSettingsDialog d(this);
  if (d.exec())
  {
    Core::instance()->restoreSettings();
    restore();
  }
}

void MainWindow::visitWebsite()
{
  QDesktopServices::openUrl(QUrl("http://yandex.ru"));
}

void MainWindow::showAbout()
{
  QString aboutStr = tr("Sample text");
  QMessageBox::about(this, tr("About wwanEric"), aboutStr);
}

void MainWindow::showAboutQt()
{
  QMessageBox::aboutQt(this);
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if ((reason == QSystemTrayIcon::Trigger) || (reason == QSystemTrayIcon::DoubleClick))
  {
    onShowAction();
  }
}

void MainWindow::onShowAction()
{
  if (isHidden())
  {
    show();
    showNormal();
    activateWindow();
  }
  else
  {
    activateWindow();
  }
}

void MainWindow::onExitAction()
{
  m_exit = true;
  close();
}

/**
 * @brief MainWindow::onRemoveGroupAction
 *
 * This is called when user likes to _delete_ the connection, including it settings.
 */
void MainWindow::onRemoveGroupAction()
{
  QAction * action = qobject_cast<QAction*>(sender());
  Q_ASSERT(action);
  QUuid uuid = action->data().toUuid();
  QString name = m_connBoxes[uuid].name;

  if (QMessageBox::question(this, tr("Remove connection?"),
                        tr("Are you really like to remove connection \"%1\"?" ENDL
                           "Archived messages, stored settings and other info will be removed.")
                            .arg(name),
                        QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
    removeViewGroup(uuid, false);
  }
}

void MainWindow::onRenameGroupAction()
{
  QAction * action = qobject_cast<QAction*>(sender());
  Q_ASSERT(action);
  QUuid uuid = action->data().toUuid();
  QString name = askUserConnectionName(uuid);
  if (! name.isEmpty())
  {
    setViewGroupName(uuid, name);
  }
}

void MainWindow::onConnectionEvent(const QUuid& connectionId,
                                   Core::ConnectionEvent event,
                                   const QVariant& data)
{
  const QList<Box>& boxes = m_connBoxes[connectionId].boxes;
  Q_ASSERT(boxes.size());

  foreach (const Box& box, boxes)
  {
    box.view->processConnectionEvent(event, data);
  }
}

