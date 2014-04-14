#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Core.h"
#include "Modem.h"

#include "IView.h"
#include "views/ModemStatusView.h"
#include "views/SmsView.h"
#include "views/UssdView.h"
#include "views/SettingsView.h"

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
  m_trayMenu->addAction(ui->actionUpdate);
  m_trayMenu->addAction(ui->actionExit);
  m_trayMenu->setDefaultAction(showAction);

  m_trayIcon->setContextMenu(m_trayMenu);

  // main menu actions
  connect(ui->actionUpdate, SIGNAL(triggered()), SLOT(updateActionTriggered()));
  connect(ui->actionExit, SIGNAL(triggered()), SLOT(onExitAction()));
  connect(ui->actionAdd_connection, SIGNAL(triggered()), SLOT(addConnection()));
  connect(ui->actionPreferences, SIGNAL(triggered()), SLOT(showPreferences()));
  connect(ui->actionVisit_website, SIGNAL(triggered()), SLOT(visitWebsite()));
  connect(ui->actionAbout, SIGNAL(triggered()), SLOT(showAbout()));
  connect(ui->actionAbout_Qt, SIGNAL(triggered()), SLOT(showAboutQt()));

  setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
  setTabShape(QTabWidget::Triangular);
  setDocumentMode(true);

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

  views.append(new ModemStatusView(this));
  views.append(new SmsView(this));
  views.append(new UssdView(this));
  views.append(new SettingsView(this));

  Settings set;

  set.beginGroup(QString(SET_GROUP_PREFIX) + groupName);

  foreach(IView* view, views)
  {
    view->init();
    view->restore(set);
  }

  set.endGroup();

  return views;
}

void MainWindow::addViewGroup(const QString &groupName)
{
  // create new one
  if(!m_boxes.contains(groupName))
  {
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

      const QString dockStyleSheet =
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

      dockWidget->setStyleSheet(dockStyleSheet);

      dockWidget->setWidget(view->widget());
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
    connect(removeAction, SIGNAL(triggered()), SLOT(onRemoveGroupAction()));
  }
}

void MainWindow::removeViews(const QList<Box> & boxes, const QString& groupName)
{
  Settings set;
  set.beginGroup(QString(SET_GROUP_PREFIX) + groupName);

  foreach(const Box &box, boxes)
  {
    IView * view = box.view;
    view->store(set);
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
}

void MainWindow::store()
{
  Settings set;

  set.beginGroup(SET_MAINWINDOW_GROUP);
  set.setValue(SET_MAINWINDOW_MINIMIZE_ON_CLOSE, m_minimizeOnClose ? 1 : 0);

  QStringList groups = m_boxes.uniqueKeys();
  set.setValue("groups", groups);
  set.endGroup();
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
  }
}

void MainWindow::init()
{
  Modem * modem = Core::instance()->modem();
  connect(modem, SIGNAL(updatedPortStatus(bool)),
          this, SLOT(updatePortStatus(bool)));

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

void MainWindow::updateActionTriggered()
{
  ContainerHash::const_iterator iter = m_boxes.constBegin();
  ContainerHash::const_iterator iterEnd = m_boxes.constEnd();

  while(iter != iterEnd)
  {
    IView * view = iter.value().view;
    QEvent * event = new QEvent(ModemEventType);

    QCoreApplication::postEvent(view->widget(), event);

    ++iter;
  }
}

void MainWindow::updatePortStatus(bool opened)
{
  ui->actionUpdate->setEnabled(opened);

  if (opened)
  {
    updateActionTriggered();
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

