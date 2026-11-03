#include <actcraft/core/printlayout.h>
#include <actcraft/print/printer.h>
#include <actcraft/ui/printcontroller.h>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QSettings>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>
#include <memory>

namespace actcraft::ui {

namespace {

// Feuille de la boîte et de l'aperçu : A4 portrait, comme le papier d'une imprimante neuve
// (print::printDocument prend le papier de l'imprimante ; le nombre de feuilles peut différer sur un autre
// papier).
constexpr core::SizeF kSheetMm{210.0, 297.0};
constexpr int kFallbackDpi = 150; // PDF écrit sans QPrinter : A4 = 1240 × 1754 px
const char* const kProviderId = "printpreview";

QString rangeName(core::PrintRange r)
{
    return r == core::PrintRange::Current  ? QStringLiteral("current")
           : r == core::PrintRange::FromTo ? QStringLiteral("from-to")
                                           : QStringLiteral("all");
}

QString notesName(core::PrintNotes n)
{
    return n == core::PrintNotes::NotesOnly   ? QStringLiteral("only")
           : n == core::PrintNotes::WithPages ? QStringLiteral("with-pages")
                                              : QStringLiteral("none");
}

QString rangeOrAll(const QString& v)
{
    return v == QLatin1String("current") || v == QLatin1String("from-to") ? v : QStringLiteral("all");
}

QString notesOrNone(const QString& v)
{
    return v == QLatin1String("only") || v == QLatin1String("with-pages") ? v : QStringLiteral("none");
}

// Réglages persistants : le fichier `settingsFile` s'il est posé ; sinon ceux de l'application, mais
// seulement si elle a un nom d'organisation (app/main.cpp) : le banc, les captures et les tests n'écrivent
// jamais dans un profil réel. Même règle que ToolHost.
std::unique_ptr<QSettings> openSettings(const QString& file)
{
    if (!file.isEmpty()) {
        return std::make_unique<QSettings>(file, QSettings::IniFormat);
    }
    if (QCoreApplication::organizationName().isEmpty()) {
        return nullptr;
    }
    return std::make_unique<QSettings>();
}

// Image de l'aperçu pour QML : « index/dpi/révision ». Chargée de façon synchrone (fil de l'interface), une
// feuille à la fois.
class PreviewProvider : public QQuickImageProvider
{
public:
    explicit PreviewProvider(PrintController* owner)
        : QQuickImageProvider(QQuickImageProvider::Image), m_owner(owner)
    {}
    QImage requestImage(const QString& id, QSize* size, const QSize&) override
    {
        const QStringList parts = id.split(QLatin1Char('/'));
        QImage image;
        if (m_owner && parts.size() >= 2) {
            image = m_owner->previewSheet(parts[0].toInt(), parts[1].toInt());
        }
        if (size) {
            *size = image.size();
        }
        return image;
    }

private:
    QPointer<PrintController> m_owner;
};

} // namespace

PrintController::PrintController(QObject* parent) : QObject(parent)
{
    loadSettings();
}

void PrintController::componentComplete()
{
    if (QQmlEngine* engine = qmlEngine(this)) {
        engine->addImageProvider(QString::fromLatin1(kProviderId), new PreviewProvider(this));
    }
}

void PrintController::invalidate()
{
    m_cache.clear();
    ++m_revision;
}

void PrintController::changed()
{
    invalidate();
    emit settingsChanged();
}

DocumentController* PrintController::document() const
{
    return m_document;
}

void PrintController::setDocument(DocumentController* document)
{
    if (document == m_document) {
        return;
    }
    if (m_document) {
        m_document->disconnect(this);
    }
    m_document = document;
    if (m_document) {
        connect(m_document, &DocumentController::documentChanged, this, &PrintController::changed);
        connect(m_document, &DocumentController::pagesChanged, this, &PrintController::changed);
        connect(m_document, &DocumentController::pageContentChanged, this, &PrintController::changed);
    }
    invalidate();
    emit documentChanged();
    emit settingsChanged();
}

int PrintController::rows() const
{
    return m_rows;
}

void PrintController::setRows(int value)
{
    value = std::clamp(value, 1, 20);
    if (value != m_rows) {
        m_rows = value;
        changed();
    }
}

int PrintController::columns() const
{
    return m_columns;
}

void PrintController::setColumns(int value)
{
    value = std::clamp(value, 1, 20);
    if (value != m_columns) {
        m_columns = value;
        changed();
    }
}

bool PrintController::frames() const
{
    return m_frames;
}

void PrintController::setFrames(bool value)
{
    if (value != m_frames) {
        m_frames = value;
        changed();
    }
}

bool PrintController::pageNumbers() const
{
    return m_pageNumbers;
}

void PrintController::setPageNumbers(bool value)
{
    if (value != m_pageNumbers) {
        m_pageNumbers = value;
        changed();
    }
}

QString PrintController::range() const
{
    return m_range;
}

void PrintController::setRange(const QString& value)
{
    const QString v = rangeOrAll(value);
    if (v != m_range) {
        m_range = v;
        changed();
    }
}

int PrintController::from() const
{
    return m_from;
}

void PrintController::setFrom(int value)
{
    value = std::clamp(value, 1, 9999);
    if (value != m_from) {
        m_from = value;
        changed();
    }
}

int PrintController::to() const
{
    return m_to;
}

void PrintController::setTo(int value)
{
    value = std::clamp(value, 1, 9999);
    if (value != m_to) {
        m_to = value;
        changed();
    }
}

int PrintController::copies() const
{
    return m_copies;
}

void PrintController::setCopies(int value)
{
    value = std::clamp(value, 1, 999);
    if (value != m_copies) {
        m_copies = value;
        changed();
    }
}

double PrintController::marginMm() const
{
    return m_marginMm;
}

void PrintController::setMarginMm(double value)
{
    value = std::clamp(value, 0.0, 100.0);
    if (value != m_marginMm) {
        m_marginMm = value;
        changed();
    }
}

bool PrintController::gray() const
{
    return m_gray;
}

void PrintController::setGray(bool value)
{
    if (value != m_gray) {
        m_gray = value;
        changed();
    }
}

QString PrintController::notes() const
{
    return m_notes;
}

void PrintController::setNotes(const QString& value)
{
    const QString v = notesOrNone(value);
    if (v != m_notes) {
        m_notes = v;
        changed();
    }
}

bool PrintController::noteBreak() const
{
    return m_noteBreak;
}

void PrintController::setNoteBreak(bool value)
{
    if (value != m_noteBreak) {
        m_noteBreak = value;
        changed();
    }
}

bool PrintController::noteSeparator() const
{
    return m_noteSeparator;
}

void PrintController::setNoteSeparator(bool value)
{
    if (value != m_noteSeparator) {
        m_noteSeparator = value;
        changed();
    }
}

QString PrintController::printer() const
{
    return m_printer;
}

void PrintController::setPrinter(const QString& value)
{
    if (value != m_printer) {
        m_printer = value;
        emit settingsChanged(); // le choix de l'imprimante ne change ni la mise en page ni l'aperçu
    }
}

QStringList PrintController::printers() const
{
    return print::printerNames();
}

namespace {

core::PrintSettings toCore(const PrintController& p)
{
    core::PrintSettings s;
    s.rows = p.rows();
    s.columns = p.columns();
    s.frames = p.frames();
    s.pageNumbers = p.pageNumbers();
    s.range = p.range() == QLatin1String("current")   ? core::PrintRange::Current
              : p.range() == QLatin1String("from-to") ? core::PrintRange::FromTo
                                                      : core::PrintRange::All;
    s.from = p.from();
    s.to = p.to();
    s.copies = p.copies();
    s.marginMm = p.marginMm();
    s.gray = p.gray();
    s.notes = p.notes() == QLatin1String("only")         ? core::PrintNotes::NotesOnly
              : p.notes() == QLatin1String("with-pages") ? core::PrintNotes::WithPages
                                                         : core::PrintNotes::None;
    s.noteBreak = p.noteBreak();
    s.noteSeparator = p.noteSeparator();
    return s;
}

} // namespace

int PrintController::sheetCount() const
{
    if (!m_document) {
        return 0;
    }
    return core::sheetCount(m_document->document(), toCore(*this),
                            static_cast<std::size_t>(std::max(0, m_document->currentIndex())), kSheetMm);
}

QString PrintController::settingsFile() const
{
    return m_settingsFile;
}

void PrintController::setSettingsFile(const QString& path)
{
    if (path == m_settingsFile) {
        return;
    }
    m_settingsFile = path;
    loadSettings();
    emit settingsFileChanged();
}

bool PrintController::available() const
{
    return print::printingAvailable();
}

int PrintController::previewRenders() const
{
    return m_previewRenders;
}

QString PrintController::previewSource(int index, int dpi) const
{
    return QStringLiteral("image://%1/%2/%3/%4")
        .arg(QString::fromLatin1(kProviderId))
        .arg(index)
        .arg(dpi)
        .arg(m_revision);
}

// L'aperçu est la feuille de print::renderSheet, jamais un second dessin. Paresseux : seule la feuille
// demandée est composée (renderSheet décode toutes les images du document à chaque appel), et gardée jusqu'au
// prochain changement de réglage ou de document.
QImage PrintController::previewSheet(int index, int dpi)
{
    if (!m_document || index < 0 || dpi <= 0) {
        return {};
    }
    const core::PrintSettings settings = toCore(*this);
    const std::vector<core::Sheet> sheets =
        core::layoutSheets(m_document->document(), settings,
                           static_cast<std::size_t>(std::max(0, m_document->currentIndex())), kSheetMm);
    if (sheets.empty() || index >= static_cast<int>(sheets.size()) * settings.copies) {
        return {};
    }
    const int distinct = index % static_cast<int>(sheets.size()); // les copies sont identiques
    const qint64 key = static_cast<qint64>(distinct) * 100000 + dpi;
    const auto hit = m_cache.constFind(key);
    if (hit != m_cache.constEnd()) {
        return *hit;
    }
    const QImage image = print::renderSheet(
        m_document->document(), sheets[static_cast<std::size_t>(distinct)], settings, kSheetMm, dpi);
    ++m_previewRenders;
    emit previewRendersChanged();
    m_cache.insert(key, image);
    return image;
}

bool PrintController::report(bool ok, const QString& message)
{
    if (ok) {
        emit finished(message);
    } else {
        emit failed(message);
    }
    return ok;
}

// Windows (QPrinter construit) : print::printDocument. Sans QPrinter, un fichier de sortie reçoit les
// feuilles de renderSheet dans un PDF (QPdfWriter) ; sans fichier, le PDF est temporaire et s'ouvre dans le
// lecteur du système (même gestionnaire que FileExchange::printThroughPdf), pour l'impression qu'il propose.
bool PrintController::print(const QString& outputFile)
{
    if (!m_document) {
        return report(false, tr("Aucun document à imprimer."));
    }
    const core::PrintSettings settings = toCore(*this);
    const std::size_t current = static_cast<std::size_t>(std::max(0, m_document->currentIndex()));
    if (print::printingAvailable()) {
        print::PrintJob job;
        job.printer = m_printer.isEmpty() && outputFile.isEmpty() ? print::defaultPrinterName() : m_printer;
        job.outputFile = outputFile;
        job.settings = settings;
        const print::PrintResult r = print::printDocument(m_document->document(), current, job);
        return report(r.ok,
                      r.ok ? tr("%n feuille(s) envoyée(s) à l'impression.", nullptr, r.sheets) : r.error);
    }
    QString target = outputFile;
    const bool openAfter = outputFile.isEmpty();
    if (openAfter) {
        QTemporaryFile file(QDir::temp().absoluteFilePath(QStringLiteral("ActCraft-impression-XXXXXX.pdf")));
        file.setAutoRemove(false);
        if (!file.open()) {
            return report(false, tr("Impression impossible : le dossier temporaire n'est pas accessible."));
        }
        target = file.fileName();
        file.close();
    }
    // Un échec ne laisse pas de PDF temporaire derrière lui (un fichier choisi par l'appelant reste à lui).
    const auto abort = [&](const QString& message) {
        if (openAfter) {
            QFile::remove(target);
        }
        return report(false, message);
    };
    const std::vector<core::Sheet> sheets =
        core::layoutSheets(m_document->document(), settings, current, kSheetMm);
    if (sheets.empty()) {
        return abort(tr("Rien à imprimer."));
    }
    QPdfWriter writer(target);
    writer.setResolution(kFallbackDpi);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.setCreator(QStringLiteral("ActCraft"));
    QPainter painter;
    if (!painter.begin(&writer)) {
        return abort(tr("Impossible d'écrire %1.").arg(target));
    }
    bool first = true;
    int written = 0;
    for (int copy = 0; copy < settings.copies; ++copy) {
        for (const core::Sheet& sheet : sheets) {
            if (!first) {
                writer.newPage();
            }
            first = false;
            const QImage image =
                print::renderSheet(m_document->document(), sheet, settings, kSheetMm, kFallbackDpi);
            painter.drawImage(QRect(0, 0, painter.device()->width(), painter.device()->height()), image);
            ++written;
        }
    }
    painter.end();
    if (!QFileInfo::exists(target)) {
        return abort(tr("Impossible d'écrire %1.").arg(target));
    }
    if (!openAfter) {
        return report(true, tr("%n feuille(s) écrite(s).", nullptr, written));
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(target))) {
        return abort(tr("Impression impossible : aucun lecteur de PDF n'a pu ouvrir %1.").arg(target));
    }
    return report(true, tr("PDF ouvert pour l'impression."));
}

void PrintController::saveSettings()
{
    const std::unique_ptr<QSettings> store = openSettings(m_settingsFile);
    if (!store) {
        return;
    }
    store->beginGroup(QStringLiteral("print"));
    store->setValue(QStringLiteral("rows"), m_rows);
    store->setValue(QStringLiteral("columns"), m_columns);
    store->setValue(QStringLiteral("frames"), m_frames);
    store->setValue(QStringLiteral("pageNumbers"), m_pageNumbers);
    store->setValue(QStringLiteral("range"), m_range);
    store->setValue(QStringLiteral("from"), m_from);
    store->setValue(QStringLiteral("to"), m_to);
    store->setValue(QStringLiteral("copies"), m_copies);
    store->setValue(QStringLiteral("marginMm"), m_marginMm);
    store->setValue(QStringLiteral("gray"), m_gray);
    store->setValue(QStringLiteral("notes"), m_notes);
    store->setValue(QStringLiteral("noteBreak"), m_noteBreak);
    store->setValue(QStringLiteral("noteSeparator"), m_noteSeparator);
    store->setValue(QStringLiteral("printer"), m_printer);
    store->endGroup();
    store->sync();
}

void PrintController::loadSettings()
{
    const core::PrintSettings d;
    const std::unique_ptr<QSettings> store = openSettings(m_settingsFile);
    if (store) {
        store->beginGroup(QStringLiteral("print"));
    }
    const auto read = [&](const char* key, const QVariant& fallback) {
        return store ? store->value(QString::fromLatin1(key), fallback) : fallback;
    };
    m_rows = std::clamp(read("rows", d.rows).toInt(), 1, 20);
    m_columns = std::clamp(read("columns", d.columns).toInt(), 1, 20);
    m_frames = read("frames", d.frames).toBool();
    m_pageNumbers = read("pageNumbers", d.pageNumbers).toBool();
    m_range = rangeOrAll(read("range", rangeName(d.range)).toString());
    m_from = std::clamp(read("from", d.from).toInt(), 1, 9999);
    m_to = std::clamp(read("to", d.to).toInt(), 1, 9999);
    m_copies = std::clamp(read("copies", d.copies).toInt(), 1, 999);
    m_marginMm = std::clamp(read("marginMm", d.marginMm).toDouble(), 0.0, 100.0);
    m_gray = read("gray", d.gray).toBool();
    m_notes = notesOrNone(read("notes", notesName(d.notes)).toString());
    m_noteBreak = read("noteBreak", d.noteBreak).toBool();
    m_noteSeparator = read("noteSeparator", d.noteSeparator).toBool();
    m_printer = read("printer", QString()).toString();
    changed();
}

} // namespace actcraft::ui
