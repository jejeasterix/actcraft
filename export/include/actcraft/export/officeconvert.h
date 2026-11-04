#pragma once
#include <QFuture>
#include <QString>
#include <QStringList>

#include <optional>

namespace actcraft::exporting {

struct Converter
{
    QString program;
    // Sans pathsByEnvironment : {in} (présentation), {out} (PDF attendu) et {outdir} (dossier de {out})
    // remplacés. Avec : aucun remplacement ; les chemins passent par ACTCRAFT_CONVERT_IN et
    // ACTCRAFT_CONVERT_OUT.
    QStringList arguments;
    QString name;
    bool pathsByEnvironment = false; // vrai pour PowerPoint
    QString
        scriptResource; // ressource Qt copiée dans le dossier temporaire puis passée par -File (PowerPoint)
};
// Ordre : (1) ACTCRAFT_OFFICE_CONVERTER (programme puis arguments séparés par « | », remplacement des
// chemins) ; (2) si ACTCRAFT_POWERSHELL est posée (variable de diagnostic documentée, coutures de test), voie
// PowerPoint prise directement, sans contrôle de présence, avec ce programme ; (3) LibreOffice (`soffice`) ;
// (4) PowerPoint, présent si la clé HKCR\PowerPoint.Application existe (QSettings, format natif), lancé par
// le chemin absolu de powershell.exe (dossier système, puis son sous-dossier WindowsPowerShell\v1.0\) ; voie
// PowerPoint : pathsByEnvironment vrai, scriptResource = ":/actcraft/export/pptx-to-pdf.ps1". Hors Windows :
// (1) et (3) seulement (PowerPoint n'existe que sous Windows ; la voie (2) y reste une couture de test).
std::optional<Converter> findConverter();
struct ConvertResult
{
    bool ok = false;
    QString error;
    QString pdfPath;
};
QFuture<ConvertResult> convertToPdf(const Converter& converter,
                                    const QString& presentationPath); // 120 s au plus

} // namespace actcraft::exporting
