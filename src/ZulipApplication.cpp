#include "ZulipApplication.h"

#include <QSettings>

ZulipApplication::ZulipApplication(int &argc, char **argv)
    :  QApplication(argc, argv),
      m_mw(0),
      m_debugMode(false),
      m_explicitDomain(false)
{
}


bool ZulipApplication::explicitDomain() const {
    return m_explicitDomain;
}

void ZulipApplication::setExplicitDomain(const QString &domain) {
    QSettings s;

    if (domain.isEmpty()) {
        s.remove("Domain");
        m_explicitDomain = false;
    } else {
        s.setValue("Domain", domain);
        m_explicitDomain = true;
    }
}
