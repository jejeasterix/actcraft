#include <actcraft/ui/spelling.h>

#include <QAbstractTextDocumentLayout>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextLine>

#include <algorithm>

namespace actcraft::ui {

namespace {

constexpr int kMaxSuggestions = 8;
constexpr int kReplaceAllLimit = 100000;

// Hunspell lit des chemins de fichiers : la ressource est copiée dans `folder` (une fois, tant que la taille
// ne change pas).
bool copyResource(const QString& name, const QString& folder, QString& target)
{
    QFile in(QStringLiteral(":/actcraft/dictionaries/") + name);
    if (!in.open(QIODevice::ReadOnly)) {
        return false;
    }
    target = folder + QLatin1Char('/') + name;
    const QFileInfo existing(target);
    if (existing.exists() && existing.size() == in.size()) {
        return true;
    }
    QFile out(target);
    return out.open(QIODevice::WriteOnly | QIODevice::Truncate) && out.write(in.readAll()) == in.size();
}

QString appDataFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           QStringLiteral("/dictionnaire-utilisateur.txt");
}

} // namespace

Spelling::Spelling(QObject* parent) : QObject(parent) {}
Spelling::~Spelling() = default;

void Spelling::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    m_document = document;
    emit wiringChanged();
}

void Spelling::setProfile(ProfileController* profile)
{
    if (m_profile == profile) {
        return;
    }
    m_profile = profile;
    emit wiringChanged();
}

QString Spelling::userDictionaryPath() const
{
    return m_userDictionaryPath.isEmpty() ? appDataFile() : m_userDictionaryPath;
}

void Spelling::setUserDictionaryPath(const QString& path)
{
    if (m_userDictionaryPath == path) {
        return;
    }
    m_userDictionaryPath = path;
    m_checker.reset(); // le moteur relira le fichier à la prochaine vérification
    m_engine.reset();
    emit wiringChanged();
}

QString Spelling::dictionaryFolder() const
{
    return m_dictionaryFolder.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                              QStringLiteral("/dictionaries")
                                        : m_dictionaryFolder;
}

void Spelling::setDictionaryFolder(const QString& folder)
{
    if (m_dictionaryFolder == folder) {
        return;
    }
    m_dictionaryFolder = folder;
    m_checker.reset();
    m_engine.reset();
    emit wiringChanged();
}

void Spelling::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    emit wiringChanged();
    if (!active) {
        showPendingPage();
    }
}

QString Spelling::word() const
{
    return m_hit ? QString::fromStdString(m_hit->word) : QString();
}

bool Spelling::prepare()
{
    if (!m_engine) {
        const QString folder = dictionaryFolder();
        QString aff;
        QString dic;
        if (!QDir().mkpath(folder) || !copyResource(QStringLiteral("fr.aff"), folder, aff) ||
            !copyResource(QStringLiteral("fr.dic"), folder, dic)) {
            m_message = tr("Dictionnaire indisponible");
            return false;
        }
        m_engine = core::makeHunspellEngine(aff.toStdString(), dic.toStdString());
        if (!m_engine) {
            m_message = tr("Dictionnaire indisponible");
            return false;
        }
        core::loadUserDictionary(*m_engine, userDictionaryPath().toStdString());
    }
    const auto flag = [this](const char* key, bool fallback) {
        return m_profile ? m_profile->setting(QStringLiteral("spelling"), QString::fromLatin1(key), fallback)
                               .toBool()
                         : fallback;
    };
    m_rules.ignoreCapitalized = flag("ignore-capitalized", false);
    m_rules.ignoreUppercase = flag("ignore-uppercase", false);
    m_rules.ignoreDigits = flag("ignore-digits", false);
    m_rules.ignoreMixedCase = flag("ignore-mixed-case", false);
    m_rules.ignoreAddresses = flag("ignore-addresses", false);
    m_rules.flagRepeated = flag("flag-repeated", true);
    m_checker = std::make_unique<core::SpellChecker>(*m_engine, m_rules);
    return true;
}

void Spelling::start()
{
    m_hit.reset();
    m_finished = false;
    m_message.clear();
    m_pendingPage = -1;
    m_suggestions.clear();
    m_highlight = QRectF();
    if (!m_document || !prepare()) {
        emit stateChanged();
        return;
    }
    advance();
}

void Spelling::advance()
{
    if (!m_document || !m_checker) {
        return;
    }
    publish(m_checker->next(m_document->document()));
}

void Spelling::publish(std::optional<core::SpellHit> hit)
{
    m_hit = std::move(hit);
    m_pendingPage = -1;
    m_suggestions.clear();
    if (m_hit) {
        m_finished = false;
        m_message.clear();
        if (!m_hit->repeated) {
            for (const std::string& s : m_engine->suggest(m_hit->word)) {
                if (m_suggestions.size() >= kMaxSuggestions) {
                    break;
                }
                m_suggestions.push_back(QString::fromStdString(s));
            }
        }
        m_highlight = highlightOf(*m_hit);
        if (m_hit->page != m_document->currentIndex()) {
            // La page en cours d'écriture ne change pas pendant un contact : le changement attend le relâché.
            if (m_contactActive) {
                m_pendingPage = m_hit->page;
            } else {
                m_document->showPage(m_hit->page);
            }
        }
    } else {
        m_finished = true;
        m_message = tr("Vérification terminée");
        m_highlight = QRectF();
    }
    emit stateChanged();
}

void Spelling::showPendingPage()
{
    if (m_pendingPage >= 0 && m_hit && m_document) {
        const int page = m_pendingPage;
        m_pendingPage = -1;
        m_document->showPage(page);
    }
}

void Spelling::ignore()
{
    if (!m_hit || !m_checker) {
        return;
    }
    m_checker->ignore();
    advance();
}

void Spelling::ignoreAlways()
{
    if (!m_hit || !m_checker) {
        return;
    }
    m_checker->ignoreAlways(m_hit->word);
    advance();
}

void Spelling::addToDictionary()
{
    if (!m_hit || !m_engine) {
        return;
    }
    core::addToUserDictionary(*m_engine, userDictionaryPath().toStdString(), m_hit->word);
    advance();
}

bool Spelling::replaceCurrent(const QString& replacement)
{
    if (!m_hit || !m_checker || !m_document) {
        return false;
    }
    std::unique_ptr<core::UndoCommand> command =
        m_checker->replace(m_document->document(), *m_hit, replacement.toStdString());
    if (!command) {
        return false;
    }
    m_document->execute(std::move(command));
    return true;
}

void Spelling::replace(const QString& replacement)
{
    if (!m_hit) {
        return;
    }
    if (!replaceCurrent(replacement)) {
        m_checker->ignore(); // le mot n'est plus à cette place
    }
    advance();
}

void Spelling::replaceAll(const QString& replacement)
{
    if (!m_hit || !m_engine || !m_document) {
        return;
    }
    const QString target = word().toLower();
    if (!replaceCurrent(replacement)) {
        m_checker->ignore();
    }
    // Les autres occurrences, avec un second vérificateur : celui de la boîte garde son curseur.
    core::SpellChecker scan(*m_engine, m_rules);
    std::optional<core::SpellHit> done;
    for (int guard = 0; guard < kReplaceAllLimit; ++guard) {
        const std::optional<core::SpellHit> hit = scan.next(m_document->document());
        if (!hit) {
            break;
        }
        const bool again = done && done->page == hit->page && done->object == hit->object &&
                           done->start == hit->start; // le remplaçant est lui-même signalé
        if (!again && !hit->repeated && QString::fromStdString(hit->word).toLower() == target) {
            if (std::unique_ptr<core::UndoCommand> command =
                    scan.replace(m_document->document(), *hit, replacement.toStdString())) {
                m_document->execute(std::move(command));
                done = hit;
                continue;
            }
        }
        scan.ignore();
    }
    advance();
}

void Spelling::close()
{
    m_hit.reset();
    m_finished = false;
    m_message.clear();
    m_pendingPage = -1;
    m_suggestions.clear();
    m_highlight = QRectF();
    emit stateChanged();
}

// Emprise du mot dans la page : mise en page du texte de l'objet à la largeur de son cadre, puis transformée.
QRectF Spelling::highlightOf(const core::SpellHit& hit) const
{
    if (!m_document || hit.page < 0 ||
        static_cast<std::size_t>(hit.page) >= m_document->document().pageCount()) {
        return {};
    }
    for (const core::PageObject& object :
         m_document->document().page(static_cast<std::size_t>(hit.page)).objects) {
        if (object.common.id != hit.object) {
            continue;
        }
        const core::Text* text = std::get_if<core::Text>(&core::body(object));
        if (text == nullptr) {
            return {};
        }
        QTextDocument layout;
        layout.setDocumentMargin(0);
        layout.setHtml(QString::fromStdString(text->html));
        layout.setTextWidth(text->bounds.width);
        layout.documentLayout()->documentSize();
        QRectF local(0, 0, text->bounds.width, text->bounds.height);
        const int from =
            static_cast<int>(hit.start); // texte brut de core : mêmes positions que QTextDocument
        const QTextBlock block = layout.findBlock(from);
        const QTextLine line =
            block.isValid() ? block.layout()->lineForTextPosition(from - block.position()) : QTextLine();
        if (line.isValid()) {
            const int rel = from - block.position();
            const qreal x1 = line.cursorToX(rel);
            const bool sameLine = rel + static_cast<int>(hit.length) <= line.textStart() + line.textLength();
            const qreal x2 = sameLine ? line.cursorToX(rel + static_cast<int>(hit.length))
                                      : line.x() + line.naturalTextWidth();
            const QRectF blockRect = layout.documentLayout()->blockBoundingRect(block);
            local = QRectF(blockRect.x() + x1, blockRect.y() + line.y(), std::max<qreal>(x2 - x1, 1.0),
                           line.height());
        }
        const core::Transform& t = object.common.transform;
        const core::PointF corners[] = {
            t.map({text->bounds.x + local.left(), text->bounds.y + local.top()}),
            t.map({text->bounds.x + local.right(), text->bounds.y + local.top()}),
            t.map({text->bounds.x + local.right(), text->bounds.y + local.bottom()}),
            t.map({text->bounds.x + local.left(), text->bounds.y + local.bottom()})};
        double minX = corners[0].x;
        double maxX = corners[0].x;
        double minY = corners[0].y;
        double maxY = corners[0].y;
        for (const core::PointF& c : corners) {
            minX = std::min(minX, c.x);
            maxX = std::max(maxX, c.x);
            minY = std::min(minY, c.y);
            maxY = std::max(maxY, c.y);
        }
        return QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    return {};
}

} // namespace actcraft::ui
