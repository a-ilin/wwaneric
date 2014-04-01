#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Modem.h"
#include "Settings.h"

#include "IView.h"
#include "views/ModemStatusView.h"
#include "views/SmsView.h"
#include "views/UssdView.h"
#include "views/SettingsView.h"


#include <QMdiSubWindow>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  connect(ui->actionUpdate, SIGNAL(triggered()), this, SLOT(updateActionTriggered()));
}

MainWindow::~MainWindow()
{
  QList<QString> groups = m_boxes.keys();
  foreach(const QString &group, groups)
  {
    removeViewGroup(group);
  }

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

QList<IView *> MainWindow::createViews()
{
  QList<IView *> views;
  views.append(new ModemStatusView(this));
  views.append(new SmsView(this));
  views.append(new UssdView(this));
  views.append(new SettingsView(this));

  Settings set;

  foreach(IView* view, views)
  {
    view->init();
    view->restore(set);
  }

  return views;
}

void MainWindow::addViewGroup(const QString &groupName)
{
  QTabWidget * container = m_boxes.value(groupName);

  // create new one
  if(!container)
  {
    container = new QTabWidget(this);
    connect(container, SIGNAL(destroyed(QObject*)), this, SLOT(containerDestroyed(QObject*)));
    container->setTabShape(QTabWidget::Triangular);
    container->setTabsClosable(false);
    container->setMovable(false);

    m_boxes.insert(groupName, container);

    QMdiSubWindow* subWindow =  ui->mdiArea->addSubWindow(container);
    subWindow->setWindowTitle(groupName);
    subWindow->showMaximized();

    QList<IView *> views = createViews();

    foreach(IView* view, views)
    {
      m_views.insertMulti(container, view);
      container->addTab(view->widget(), view->name());
    }
  }
}

void MainWindow::removeViews(QTabWidget *container)
{
  Q_ASSERT(container);

  Settings set;
  QList<IView *> views = m_views.values(container);
  foreach(IView* view, views)
  {
    view->store(set);
    view->tini();
    delete view;
  }

  m_views.remove(container);
}

void MainWindow::removeViewGroup(const QString &groupName)
{
  QTabWidget * container = m_boxes.value(groupName);

  if (container)
  {
    m_boxes.remove(groupName);
    ui->mdiArea->removeSubWindow(container);

    removeViews(container);

    delete container;
  }
}

void MainWindow::containerDestroyed(QObject *obj)
{
  QTabWidget * container = static_cast<QTabWidget*>(obj);

  ContainerHash::iterator iter = m_boxes.begin();
  ContainerHash::const_iterator iterEnd = m_boxes.constEnd();

  while(iter != iterEnd)
  {
    if(iter.value() == container)
    {
      m_boxes.erase(iter);
      break;
    }
  }

  removeViews(container);
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

