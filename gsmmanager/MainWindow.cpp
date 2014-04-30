#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Modem.h"

#include "IView.h"
#include "ModemStatusView.h"
#include "SmsView.h"
#include "UssdView.h"
#include "SettingsView.h"

#include "ModemStatus.h"

#include "AppSettingsDialog.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QDockWidget>
#include <QInputDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QUrl>

#ifdef QT_DEBUG
#include "debugParsers.h"
#endif

#define SET_GROUP_PREFIX "grp_"
#define SET_VIEW_PREFIX  "view_"

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
  connect(Core::instance(), SIGNAL(connectionEvent(QString,Core::ConnectionEvent,QVariant)),
          this, SLOT(onConnectionEvent(QString,Core::ConnectionEvent,QVariant)),
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

QList<IView *> MainWindow::createViews(const QString &groupName)
{
  QList<IView *> views;

  views.append(new ModemStatusView(groupName, this));
  views.append(new SmsView(groupName, this));
  views.append(new UssdView(groupName, this));
  views.append(new SettingsView(groupName, this));

  Settings set;

  set.beginGroup(QString(SET_GROUP_PREFIX) + groupName);

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

void MainWindow::addViewGroup(const QString &groupName)
{
  // create new one
  if(!m_boxes.contains(groupName))
  {
    // core connection
    Core::instance()->createConnection(groupName);

    // group menu
    QMenu * menuGroup = new QMenu(groupName, this);
    ui->menuConnections->addMenu(menuGroup);
    menuGroup->setObjectName(groupName);

    QList<QDockWidget*> dockWidgets;

    // views
    QList<IView *> views = createViews(groupName);
    foreach(IView* view, views)
    {
      // dock widget create
      QString name = QString("%1 - %2").arg(groupName).arg(view->name());
      QDockWidget * dockWidget = new QDockWidget(name, this);
      dockWidget->setObjectName(view->id());
      dockWidget->setWidget(view->widget());
      dockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
      addDockWidget(Qt::RightDockWidgetArea, dockWidget);
      dockWidgets.append(dockWidget);

      // index
      Box box;
      box.view = view;
      box.container = dockWidget;
      m_boxes.insertMulti(groupName, box);
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
    QAction * removeAction = menuGroup->addAction(tr("Remove connection"));
    removeAction->setData(groupName);
    removeAction->setIcon(QIcon("icons/application_delete_connection.png"));
    connect(removeAction, SIGNAL(triggered()), SLOT(onRemoveGroupAction()));

    // connection widget item
    {
      int row = ui->connectionsWidget->rowCount();
      ui->connectionsWidget->setRowCount(row + 1);

      QTableWidgetItem * itemName = new QTableWidgetItem(groupName);
      ui->connectionsWidget->setItem(row, ColumnName, itemName);

      QTableWidgetItem * itemStatus = new QTableWidgetItem(QString());
      ui->connectionsWidget->setItem(row, ColumnStatus, itemStatus);

      QTableWidgetItem * itemSignal = new QTableWidgetItem(QString());
      ui->connectionsWidget->setItem(row, ColumnSignalStrength, itemSignal);

      updateConnectionStatus(groupName, false);
      updateSignalStrength(groupName, 0.0);
    }
  }
}

void MainWindow::removeViews(const QList<Box> & boxes, const QString& groupName)
{
  Settings set;
  set.beginGroup(QString(SET_GROUP_PREFIX) + groupName);

  foreach(const Box &box, boxes)
  {
    IView * view = box.view;

    set.beginGroup(QString(SET_VIEW_PREFIX) + view->id());
    view->store(set);
    set.endGroup();

    view->tini();
    delete view;
  }

  set.endGroup();
}

void MainWindow::restore()
{
  Settings set;
  set.beginGroup(SET_MAINWINDOW_GROUP);
  m_minimizeOnClose = set.value(SET_MAINWINDOW_MINIMIZE_ON_CLOSE).toInt();
  m_exit = !m_minimizeOnClose;
  QStringList groups = set.value("groups").toStringList();
  set.endGroup();

  foreach (const QString &grp, groups)
  {
    addViewGroup(grp);
  }

  // geometry, state
  set.beginGroup(SET_MAINWINDOW_GROUP);
  restoreGeometry(set.value("windowGeometry").toByteArray());
  restoreState(set.value("windowState").toByteArray());
  set.endGroup();
}

void MainWindow::store()
{
  Settings set;

  set.beginGroup(SET_MAINWINDOW_GROUP);
  set.setValue(SET_MAINWINDOW_MINIMIZE_ON_CLOSE, m_minimizeOnClose ? 1 : 0);

  QStringList groups = m_boxes.uniqueKeys();
  set.setValue("groups", groups);

  // geometry, state
  set.setValue("windowGeometry", saveGeometry());
  set.setValue("windowState", saveState());

  set.endGroup();
}

void MainWindow::updateConnectionStatus(const QString& connectionId, bool status)
{
  QList<QTableWidgetItem*> items = ui->connectionsWidget->findItems(connectionId,
                                                                    Qt::MatchExactly);

  Q_ASSERT(items.size() == 1);
  int row = items.at(0)->row();

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

void MainWindow::updateSignalStrength(const QString& connectionId, double strengthPercent)
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

  QList<QTableWidgetItem*> items = ui->connectionsWidget->findItems(connectionId,
                                                                    Qt::MatchExactly);

  Q_ASSERT(items.size() == 1);
  int row = items.at(0)->row();

  QTableWidgetItem * signalItem = ui->connectionsWidget->item(row, ColumnSignalStrength);
  Q_ASSERT(signalItem);

  signalItem->setIcon(signalIcon);
  signalItem->setToolTip(tr("Signal strength: %1%").arg((int)strengthPercent));

  ui->connectionsWidget->resizeColumnsToContents();
}

void MainWindow::removeViewGroup(const QString &groupName)
{
  QList<Box> boxes = m_boxes.values(groupName);

  if (boxes.size())
  {
    m_boxes.remove(groupName);

    // remove dock widgets from MainWindow
    foreach(const Box &box, boxes)
    {
      removeDockWidget(box.container);
    }

    // delete views
    removeViews(boxes, groupName);

    foreach(const Box &box, boxes)
    {
      // delete dock widgets
      removeDockWidget(box.container);
    }

    // menu
    QMenu * menu = findChild<QMenu*>(groupName);
    Q_ASSERT(menu);
    menu->clear();
    ui->menuConnections->removeAction(menu->menuAction());
    menu->deleteLater();

    // core connection
    Core::instance()->removeConnection(groupName);

    // connections widget
    {
      QList<QTableWidgetItem*> items = ui->connectionsWidget->findItems(groupName,
                                                                        Qt::MatchExactly);

      Q_ASSERT(items.size() == 1);
      int row = items.at(0)->row();
      ui->connectionsWidget->removeRow(row);
    }
  }
}

void MainWindow::init()
{
  restore();
}

void MainWindow::tini()
{
  store();

  QStringList groups = m_boxes.uniqueKeys();
  foreach(const QString &group, groups)
  {
    removeViewGroup(group);
  }
}

void MainWindow::addConnection()
{
  bool ok = false;
  QString name = QInputDialog::getText(this, tr("Specify connection name"),
                                       tr("Name: "), QLineEdit::Normal, QString(), &ok);

  if (ok)
  {
    if (!name.isEmpty())
    {
      addViewGroup(name);
    }
    else
    {
      QMessageBox::critical(this, tr("Wrong value"), tr("Name cannot be empty!"));
    }
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
  QString groupName = action->data().toString();

  if (QMessageBox::question(this, tr("Remove connection?"),
                        tr("Are you really like to remove connection \"%1\"?" ENDL
                           "All locally stored settings and info will be removed.")
                            .arg(groupName),
                        QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
    removeViewGroup(groupName);
    Settings set;
    set.remove(QString(SET_GROUP_PREFIX) + groupName);
  }
}

void MainWindow::onConnectionEvent(const QString& connectionId,
                                   Core::ConnectionEvent event,
                                   const QVariant& data)
{
  QList<Box> boxes = m_boxes.values(connectionId);
  Q_ASSERT(boxes.size());

  foreach (const Box &box, boxes)
  {
    box.view->processConnectionEvent(event, data);
  }
}

