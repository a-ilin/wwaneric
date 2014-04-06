#ifndef CORE_H
#define CORE_H

#include "Modem.h"
#include "IView.h"

#include <QtGlobal>
#include <QList>
#include <QSharedPointer>

class Core
{
public:
  Core();
  ~Core();

  bool init();
  bool tini();

  static Core * instance() { return m_instance; }

  Modem* modem(const QString &id = QString()) const;

  QString appUserDirectory() const;

  ConversationHandler* conversationHandler(Modem *modem, const QString &name) const;

private:

  static Core * m_instance;

  QMap<QString, QSharedPointer<Modem> > m_modems;

  typedef QHash<QString, ConversationHandler*> ConversationHandlersHash;
  typedef QHash<Modem*,  ConversationHandlersHash> ConversationHandlersModemHash;
  ConversationHandlersModemHash m_conversationHandlers;

  Q_DISABLE_COPY(Core)
};

#endif // CORE_H
