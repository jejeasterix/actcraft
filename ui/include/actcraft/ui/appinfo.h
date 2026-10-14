#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Identité de l'application pour « À propos » (F-760, décision 34) : version, licence, page du projet,
// notices.
class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString licenseName READ licenseName CONSTANT)
    Q_PROPERTY(QString projectUrl READ projectUrl CONSTANT)
    Q_PROPERTY(QString notices READ notices CONSTANT)

public:
    explicit AppInfo(QObject* parent = nullptr);

    QString version() const;     // project(VERSION) du CMakeLists.txt racine
    QString licenseName() const; // « MIT »
    QString projectUrl() const;  // page du projet (F-760 : l'aide est la page du projet)
    // THIRD_PARTY_NOTICES.md, intégré aux ressources (:/actcraft/THIRD_PARTY_NOTICES.md).
    QString notices() const;
};

} // namespace actcraft::ui
