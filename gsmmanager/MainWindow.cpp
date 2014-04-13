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
#include <QInputDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QUrl>

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
  QTabWidget * container = m_boxes.value(groupName).container;

  // create new one
  if(!container)
  {
    container = new QTabWidget(this);
    connect(container, SIGNAL(destroyed(QObject*)), this, SLOT(containerDestroyed(QObject*)));
    container->setTabShape(QTabWidget::Triangular);
    container->setTabsClosable(false);
    container->setMovable(false);

    QMdiSubWindow* subWindow =  ui->mdiArea->addSubWindow(container);
    subWindow->setWindowTitle(groupName);
    subWindow->showMaximized();

    Box box;
    box.container = container;
    box.menuGroup = new QMenu(groupName, this);
    ui->menuConnections->addMenu(box.menuGroup);

    QList<IView *> views = createViews(groupName);
    foreach(IView* view, views)
    {
      m_views.insertMulti(container, view);
      container->addTab(view->widget(), view->name());
      box.menuGroup->addAction(view->name());
    }

    box.menuGroup->addSeparator();
    box.menuGroup->addAction(tr("Remove connection"));

    m_boxes.insert(groupName, box);
  }
}

void MainWindow::removeViews(QTabWidget *container, const QString& groupName)
{
  Q_ASSERT(container);

  Settings set;
  set.beginGroup(QString(SET_GROUP_PREFIX) + groupName);

  QList<IView *> views = m_views.values(container);
  foreach(IView* view, views)
  {
    view->store(set);
    view->tini();
    delete view;
  }

  set.endGroup();

  m_views.remove(container);
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

  QStringList groups = m_boxes.keys();
  set.setValue("groups", groups);
  set.endGroup();
}

void MainWindow::removeViewGroup(const QString &groupName)
{
  QTabWidget * container = m_boxes.value(groupName).container;

  if (container)
  {
    m_boxes.remove(groupName);
    ui->mdiArea->removeSubWindow(container);

    removeViews(container, groupName);

    delete container;
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

  QStringList groups = m_boxes.keys();
  foreach(const QString &group, groups)
  {
    removeViewGroup(group);
  }
}

void MainWindow::containerDestroyed(QObject *obj)
{
  QTabWidget * container = static_cast<QTabWidget*>(obj);

  ContainerHash::iterator iter = m_boxes.begin();
  ContainerHash::const_iterator iterEnd = m_boxes.constEnd();

  QString groupName;

  while(iter != iterEnd)
  {
    if(iter.value().container == container)
    {
      groupName = iter.key();
      iter = m_boxes.erase(iter);
      break;
    }
    else
    {
      ++iter;
    }
  }

  removeViews(container, groupName);
}

void MainWindow::updateActionTriggered()
{
  MapViews::const_iterator iter = m_views.constBegin();
  MapViews::const_iterator iterEnd = m_views.constEnd();

  while(iter != iterEnd)
  {
    IView * view = iter.value();
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

