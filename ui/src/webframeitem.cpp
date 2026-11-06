#include "actcraft/ui/webframeitem.h"

#include "actcraft/ui/documentcontroller.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QStringDecoder>
#include <QTransform>

#include <algorithm>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

using Verdict = WebContentPolicy::Verdict;

constexpr int kMaxRedirects = 10;
constexpr qint64 kMaxBody = 16 * 1024 * 1024; // une page plus lourde n'est pas montrée
const QString kLinkPrefix = QStringLiteral("actcraft-link-");

QRectF pageBox(const core::PageObject& object, const core::WebFrame& frame)
{
    const core::Transform& t = object.common.transform;
    const QTransform q(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy);
    return q.mapRect(QRectF(frame.bounds.x, frame.bounds.y, frame.bounds.width, frame.bounds.height));
}

QString escaped(const QString& text)
{
    return text.toHtmlEscaped();
}

// Valeur d'attribut HTML lue : entités courantes rendues (une adresse en `&amp;` reste la même adresse).
QString unescapedAttribute(QString value)
{
    value.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    value.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
    value.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    value.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    value.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return value.trimmed();
}

// Source d'une règle de sécurité du contenu : adresse exacte, sans « ; » ni « , » ni espace (encodés).
QString cspSource(const QUrl& url)
{
    QString s = QString::fromLatin1(url.adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery).toEncoded());
    s.replace(QLatin1Char(';'), QStringLiteral("%3B"));
    s.replace(QLatin1Char(','), QStringLiteral("%2C"));
    s.replace(QLatin1Char('\''), QStringLiteral("%27"));
    return s;
}

class Rewriter
{
public:
    Rewriter(const QUrl& base, const std::function<Verdict(const QUrl&)>& decide)
        : m_base(base), m_decide(decide)
    {}

    // Adresse passée par la politique, rendue absolue ; vide si refusée ou bloquée.
    QString allowed(const QString& raw, QStringList& into)
    {
        const QString value = unescapedAttribute(raw);
        if (value.isEmpty())
            return {};
        const QUrl url = m_base.isValid() ? m_base.resolved(QUrl(value)) : QUrl(value);
        if (!url.isValid() || url.isRelative())
            return {};
        if (m_decide(url) != Verdict::Load)
            return {};
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("data")) {
            m_data = true;
        } else if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
            into.append(cspSource(url));
        } else {
            return {}; // file, about : rien qu'une vue sans accès aux fichiers puisse lire
        }
        return QString::fromLatin1(url.toEncoded());
    }

    QString css(const QString& text, QStringList& into)
    {
        static const QRegularExpression urlRe(QStringLiteral(R"(url\(\s*(['"]?)([^'")]*)\1\s*\))"),
                                              QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression importRe(QStringLiteral(R"(@import\s+(['"])([^'"]*)\1)"),
                                                 QRegularExpression::CaseInsensitiveOption);
        QString out = replaceAll(text, urlRe, [&](const QRegularExpressionMatch& m) {
            const QString kept = allowed(m.captured(2), into);
            return kept.isEmpty() ? QStringLiteral("url(about:blank)")
                                  : QStringLiteral("url(\"%1\")").arg(kept);
        });
        out = replaceAll(out, importRe, [&](const QRegularExpressionMatch& m) {
            const QString kept = allowed(m.captured(2), m_styles);
            return kept.isEmpty() ? QStringLiteral("@import \"about:blank\"")
                                  : QStringLiteral("@import \"%1\"").arg(kept);
        });
        return out;
    }

    static QString replaceAll(const QString& text, const QRegularExpression& re,
                              const std::function<QString(const QRegularExpressionMatch&)>& with)
    {
        QString out;
        qsizetype last = 0;
        auto it = re.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += QStringView(text).mid(last, m.capturedStart() - last);
            out += with(m);
            last = m.capturedEnd();
        }
        out += QStringView(text).mid(last);
        return out;
    }

    QString tag(const QString& name, const QString& attributes, bool selfClosing)
    {
        static const QRegularExpression attrRe(
            QStringLiteral(R"re(([^\s=>/]+)(?:\s*=\s*("[^"]*"|'[^']*'|[^\s>]+))?)re"));
        const QString lower = name.toLower();
        if (lower == QLatin1String("base"))
            return {}; // la base est celle du modèle
        struct Attribute
        {
            QString name;
            QString value;
            bool hasValue = false;
        };
        QVector<Attribute> attrs;
        auto it = attrRe.globalMatch(attributes);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            Attribute a;
            a.name = m.captured(1);
            QString v = m.captured(2);
            a.hasValue = m.capturedLength(2) > 0;
            if (v.size() >= 2 && (v.front() == QLatin1Char('"') || v.front() == QLatin1Char('\'')))
                v = v.mid(1, v.size() - 2);
            a.value = v;
            attrs.append(a);
        }
        const auto valueOf = [&attrs](const QString& key) {
            for (const Attribute& a : attrs) {
                if (a.name.compare(key, Qt::CaseInsensitive) == 0)
                    return a.value;
            }
            return QString();
        };
        if (lower == QLatin1String("meta")) {
            const QString equiv = valueOf(QStringLiteral("http-equiv")).toLower();
            if (equiv == QLatin1String("refresh") || equiv == QLatin1String("set-cookie") ||
                equiv == QLatin1String("link"))
                return {}; // une navigation ou un en-tête que la politique ne verrait pas
        }
        const QString rel = valueOf(QStringLiteral("rel")).toLower();
        QString out = QLatin1Char('<') + name;
        for (Attribute& a : attrs) {
            const QString key = a.name.toLower();
            QString value = a.value;
            bool keep = true;
            if (key.startsWith(QLatin1String("on")) || key == QLatin1String("ping") ||
                key == QLatin1String("download") || key == QLatin1String("target") ||
                key == QLatin1String("formaction") || key == QLatin1String("action") ||
                key == QLatin1String("manifest") || key == QLatin1String("codebase") ||
                key == QLatin1String("archive") || key == QLatin1String("longdesc") ||
                key == QLatin1String("cite") || key == QLatin1String("lowsrc") ||
                key == QLatin1String("dynsrc")) {
                keep = false;
            } else if (key == QLatin1String("href") || key == QLatin1String("xlink:href")) {
                if (lower == QLatin1String("a") || lower == QLatin1String("area")) {
                    value = link(a.value);
                    keep = !value.isEmpty();
                } else if (lower == QLatin1String("link")) {
                    QStringList& list = rel.contains(QLatin1String("stylesheet"))
                                            ? m_styles
                                            : (rel.contains(QLatin1String("icon")) ? m_images : m_dropped);
                    value = (&list == &m_dropped) ? QString() : allowed(a.value, list);
                    keep = !value.isEmpty();
                } else {
                    value = allowed(a.value, m_images); // image d'un SVG
                    keep = !value.isEmpty();
                }
            } else if (key == QLatin1String("src") || key == QLatin1String("data") ||
                       key == QLatin1String("poster") || key == QLatin1String("background")) {
                if (lower == QLatin1String("iframe") || lower == QLatin1String("frame") ||
                    lower == QLatin1String("object") || lower == QLatin1String("embed") ||
                    lower == QLatin1String("script")) {
                    value = QStringLiteral("about:blank"); // contexte imbriqué : ni réécrit ni lu
                } else {
                    QStringList& list = (lower == QLatin1String("video") || lower == QLatin1String("audio") ||
                                         lower == QLatin1String("source") || lower == QLatin1String("track"))
                                            ? m_media
                                            : m_images;
                    value = allowed(a.value, list);
                    if (value.isEmpty())
                        value = QStringLiteral("about:blank");
                }
            } else if (key == QLatin1String("srcset") || key == QLatin1String("imagesrcset")) {
                QStringList kept;
                for (const QString& candidate : unescapedAttribute(a.value).split(QLatin1Char(','))) {
                    const QStringList parts = candidate.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
                    if (parts.isEmpty())
                        continue;
                    const QString url = allowed(parts.front(), m_images);
                    if (!url.isEmpty())
                        kept.append((QStringList{url} + parts.mid(1)).join(QLatin1Char(' ')));
                }
                value = kept.join(QStringLiteral(", "));
                keep = !value.isEmpty();
            } else if (key == QLatin1String("style")) {
                value = css(unescapedAttribute(a.value), m_images);
            }
            if (!keep)
                continue;
            out += QLatin1Char(' ') + a.name;
            if (a.hasValue || value != a.value)
                out += QStringLiteral("=\"%1\"").arg(value.toHtmlEscaped());
        }
        out += selfClosing ? QStringLiteral(" />") : QStringLiteral(">");
        return out;
    }

    // Lien : un renvoi interne vers la cible, décidée au clic ; un renvoi vers la même page reste.
    QString link(const QString& raw)
    {
        const QString value = unescapedAttribute(raw);
        if (value.startsWith(QLatin1Char('#')) && !value.startsWith(QLatin1Char('#') + kLinkPrefix))
            return value;
        const QUrl url = m_base.isValid() ? m_base.resolved(QUrl(value)) : QUrl(value);
        if (!url.isValid() || url.isRelative())
            return {};
        const QString scheme = url.scheme();
        if (scheme != QLatin1String("http") && scheme != QLatin1String("https") &&
            scheme != QLatin1String("file"))
            return {};
        m_links.append(url);
        return QLatin1Char('#') + kLinkPrefix + QString::number(m_links.size() - 1);
    }

    QString policy() const
    {
        const auto list = [](QStringList sources, const QStringList& extra = {}) {
            sources += extra;
            sources.removeDuplicates();
            return sources.isEmpty() ? QStringLiteral("'none'") : sources.join(QLatin1Char(' '));
        };
        QStringList images = m_images;
        if (m_data)
            images.append(QStringLiteral("data:"));
        return QStringLiteral("default-src 'none'; img-src %1; media-src %2; style-src 'unsafe-inline' %3; "
                              "font-src %4; script-src 'none'; connect-src 'none'; frame-src 'none'; "
                              "child-src 'none'; object-src 'none'; worker-src 'none'; manifest-src 'none'; "
                              "form-action 'none'; base-uri 'none'")
            .arg(list(images), list(m_media), list(m_styles).replace(QStringLiteral("'none'"), QString()),
                 list(m_images, m_styles));
    }

    QStringList all() const
    {
        QStringList everything = m_images + m_media + m_styles;
        everything.removeDuplicates();
        return everything;
    }

    QVector<QUrl> m_links;
    QStringList m_images;
    QStringList m_media;
    QStringList m_styles;
    QStringList m_dropped;
    bool m_data = false;

private:
    QUrl m_base;
    const std::function<Verdict(const QUrl&)>& m_decide;
};

// Image fixe du cadre : ressource (clé SHA-256) puis objet, une étape.
class SnapshotCommand final : public core::UndoCommand
{
public:
    SnapshotCommand(std::size_t page, core::PageObject object, QByteArray png)
        : m_page(page), m_object(std::move(object)), m_png(std::move(png))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const std::string key = document.addResource(m_png, "png");
        if (!m_inner) {
            if (auto* image = std::get_if<core::ImageObject>(&core::body(m_object)))
                image->resource = key;
            m_inner = std::make_unique<core::AddObjectCommand>(m_page, m_object);
        }
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    std::size_t m_page;
    core::PageObject m_object;
    QByteArray m_png;
    std::unique_ptr<core::AddObjectCommand> m_inner;
};

QString messagePage(const QString& text)
{
    return QStringLiteral("<!DOCTYPE html><html><body style=\"font-family:sans-serif;color:#555\"><p>%1</p>"
                          "</body></html>")
        .arg(escaped(text));
}

} // namespace

WebFrameItems::Prepared WebFrameItems::prepareHtml(const QString& html, const QUrl& base,
                                                   const std::function<Verdict(const QUrl&)>& decide)
{
    // Balises ouvrantes (attributs entre guillemets compris), blocs <style> et commentaires.
    static const QRegularExpression tokenRe(
        QStringLiteral(
            R"re(<!--.*?-->|<style\b[^>]*>(.*?)</style\s*>|<([a-zA-Z][a-zA-Z0-9:-]*)((?:\s+[^\s=>/]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^\s>]+))?)*)\s*(/?)>)re"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    Rewriter rewriter(base, decide);
    const QString body = Rewriter::replaceAll(html, tokenRe, [&rewriter](const QRegularExpressionMatch& m) {
        const QString whole = m.captured(0);
        if (whole.startsWith(QLatin1String("<!--")))
            return QString();
        if (m.capturedStart(1) >= 0 && whole.startsWith(QLatin1String("<style"), Qt::CaseInsensitive)) {
            const qsizetype open = whole.indexOf(QLatin1Char('>')) + 1;
            return whole.left(open) + rewriter.css(m.captured(1), rewriter.m_images) +
                   QStringLiteral("</style>");
        }
        return rewriter.tag(m.captured(2), m.captured(3), !m.captured(4).isEmpty());
    });
    Prepared prepared;
    const QString head = QStringLiteral("<meta http-equiv=\"Content-Security-Policy\" content=\"%1\">"
                                        "<meta name=\"referrer\" content=\"no-referrer\">")
                             .arg(rewriter.policy());
    // Après le doctype (sinon le moteur passerait en mode de compatibilité), avant tout le reste : le parseur
    // range ces balises dans l'en-tête, avant la première ressource.
    static const QRegularExpression doctypeRe(QStringLiteral(R"(^\s*<!doctype[^>]*>)"),
                                              QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch doctype = doctypeRe.match(body);
    if (doctype.hasMatch())
        prepared.html = body.left(doctype.capturedEnd()) + head + body.mid(doctype.capturedEnd());
    else
        prepared.html = head + body;
    prepared.links = rewriter.m_links;
    prepared.allowed = rewriter.all();
    return prepared;
}

WebFrameItems::WebFrameItems(QObject* parent) : QAbstractListModel(parent) {}

WebFrameItems::~WebFrameItems()
{
    if (!m_key.isEmpty())
        m_policy.forget(m_key);
}

DocumentController* WebFrameItems::document() const noexcept
{
    return m_document;
}

void WebFrameItems::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    for (QMetaObject::Connection& c : m_connections)
        disconnect(c);
    m_document = document;
    if (document) {
        m_connections[0] =
            connect(document, &DocumentController::documentChanged, this, &WebFrameItems::rebuild);
        m_connections[1] =
            connect(document, &DocumentController::pagesChanged, this, &WebFrameItems::rebuild);
        m_connections[2] =
            connect(document, &DocumentController::liftingChanged, this, &WebFrameItems::rebuild);
        m_connections[3] =
            connect(document, &DocumentController::fileUrlChanged, this, &WebFrameItems::rebuild);
        // Ouverture réussie : le document est reçu, même sans fichier (un .flipchart, décision 18).
        m_connections[4] =
            connect(document, &DocumentController::loadFinished, this, [this](bool ok, const QString&) {
                if (!ok || !m_document)
                    return;
                m_policy.setReceived(m_key, true);
                for (int i = 0; i < m_rows.size(); ++i) {
                    prepareRow(m_rows[i]);
                    rowChanged(i);
                }
                emit countChanged();
            });
    }
    m_generation = 0;
    m_page = -1;
    rebuild();
    emit documentChanged();
}

void WebFrameItems::documentReplaced()
{
    if (!m_key.isEmpty())
        m_policy.forget(m_key);
    m_generation = m_document->generation();
    m_key = QStringLiteral("document-%1").arg(++m_keys);
    const QUrl file = m_document->fileUrl();
    m_policy.setReceived(m_key, file.isValid() && !file.isEmpty());
    if (file.isLocalFile())
        m_policy.setResourceFolder(m_key, QFileInfo(file.toLocalFile()).absolutePath());
    m_page = -1; // tout est relu
    emit documentKeyChanged();
}

void WebFrameItems::rebuild()
{
    if (!m_document) {
        if (!m_rows.isEmpty()) {
            beginResetModel();
            m_rows.clear();
            endResetModel();
            emit countChanged();
        }
        return;
    }
    if (m_document->isLifting())
        return; // objets soulevés hors de la page : les vues restent, relues au dépôt
    if (m_key.isEmpty() || m_document->generation() != m_generation)
        documentReplaced();
    const int page = m_document->currentIndex();
    const bool samePage = page == m_page;
    m_page = page;
    QVector<Row> fresh;
    const core::Document& doc = m_document->document();
    if (page >= 0 && static_cast<std::size_t>(page) < doc.pageCount()) {
        for (const core::PageObject& object : doc.page(static_cast<std::size_t>(page)).objects) {
            const auto* frame = std::get_if<core::WebFrame>(&core::body(object));
            if (!frame || !object.common.visible)
                continue;
            Row row;
            row.id = object.common.id;
            row.bounds = pageBox(object, *frame);
            row.html = QString::fromStdString(frame->html);
            row.address = QString::fromStdString(frame->address);
            fresh.append(row);
        }
    }
    bool sameIds = samePage && fresh.size() == m_rows.size();
    for (qsizetype i = 0; sameIds && i < fresh.size(); ++i)
        sameIds = fresh[i].id == m_rows[i].id;
    if (sameIds) {
        for (int i = 0; i < m_rows.size(); ++i) {
            Row& row = m_rows[i];
            const bool contentChanged = row.html != fresh[i].html || row.address != fresh[i].address;
            if (!contentChanged && row.bounds == fresh[i].bounds)
                continue;
            row.bounds = fresh[i].bounds;
            row.html = fresh[i].html;
            row.address = fresh[i].address;
            if (contentChanged) {
                row.history.clear();
                row.historyIndex = -1;
                prepareRow(row);
            }
            rowChanged(i);
        }
        return;
    }
    // Cadres gardés d'une relecture à l'autre de la même page : leur contenu reste.
    for (Row& row : fresh) {
        const int old = samePage ? rowOf(row.id) : -1;
        if (old >= 0 && m_rows[old].html == row.html && m_rows[old].address == row.address) {
            Row kept = m_rows[old];
            kept.bounds = row.bounds;
            row = std::move(kept);
        } else {
            row.serial = 0;
            prepareRow(row);
        }
    }
    beginResetModel();
    m_rows = std::move(fresh);
    endResetModel();
    if (m_pendingRow >= m_rows.size()) {
        m_pendingRow = -1;
        emit unblockPendingChanged();
    }
    emit countChanged();
}

Verdict WebFrameItems::decide(const QUrl& url) const
{
    return m_policy.decide(url, m_key, m_page);
}

void WebFrameItems::prepareRow(Row& row)
{
    row.blocked = false;
    row.refused = false;
    row.loading = false;
    row.pendingUrl = QUrl();
    if (!row.address.trimmed().isEmpty()) {
        const QUrl url = QUrl::fromUserInput(row.address.trimmed());
        if (row.historyIndex >= 0 && row.historyIndex < row.history.size())
            navigate(row, row.history[row.historyIndex], false);
        else
            navigate(row, url, true);
        return;
    }
    showContent(row, row.html, QUrl());
}

void WebFrameItems::showContent(Row& row, const QString& html, const QUrl& base)
{
    const Prepared prepared = prepareHtml(html, base, [this](const QUrl& url) { return decide(url); });
    row.content = prepared.html;
    row.links = prepared.links;
    row.baseUrl = base;
    row.loading = false;
    ++row.serial;
}

void WebFrameItems::navigate(Row& row, const QUrl& url, bool pushHistory)
{
    if (pushHistory) {
        row.history.resize(row.historyIndex + 1);
        row.history.append(url);
        row.historyIndex = static_cast<int>(row.history.size()) - 1;
    }
    row.blocked = false;
    row.refused = false;
    row.pendingUrl = QUrl();
    switch (decide(url)) {
    case Verdict::Blocked:
        row.blocked = true;
        row.pendingUrl = url;
        row.content.clear();
        row.links.clear();
        row.loading = false;
        ++row.serial;
        return;
    case Verdict::Refuse:
        row.refused = true;
        row.content.clear();
        row.links.clear();
        row.loading = false;
        ++row.serial;
        return;
    case Verdict::Load:
        break;
    }
    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        if (file.size() <= kMaxBody && file.open(QIODevice::ReadOnly))
            showContent(row, QStringDecoder::decoderForHtml(file.peek(4096)).decode(file.readAll()), url);
        else
            showContent(row, messagePage(tr("Fichier illisible.")), QUrl());
        return;
    }
    if (url.scheme() == QLatin1String("about")) {
        showContent(row, QString(), QUrl());
        return;
    }
    fetch(row, url, 0);
}

void WebFrameItems::fetch(Row& row, const QUrl& url, int redirects)
{
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    // Chaque redirection revient ici, décidée avant d'être suivie.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    QNetworkReply* reply = m_network->get(request);
    row.loading = true;
    ++row.serial;
    m_replies.insert(reply, Pending{row.id, row.serial, redirects});
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
        if (received > kMaxBody)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { fetched(reply); });
}

void WebFrameItems::fetched(QNetworkReply* reply)
{
    reply->deleteLater();
    const Pending pending = m_replies.take(reply);
    const int index = rowOf(pending.id);
    if (index < 0 || m_rows[index].serial != pending.serial)
        return; // cadre parti ou passé à une autre adresse
    Row& row = m_rows[index];
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (status >= 300 && status < 400 && target.isValid()) {
        const QUrl next = reply->url().resolved(target);
        if (pending.redirects >= kMaxRedirects) {
            row.refused = true;
            showContent(row, messagePage(tr("Trop de redirections.")), QUrl());
        } else {
            switch (decide(next)) {
            case Verdict::Load:
                if (next.scheme() == QLatin1String("http") || next.scheme() == QLatin1String("https")) {
                    fetch(row, next, pending.redirects + 1);
                    rowChanged(index);
                    return;
                }
                row.refused = true;
                row.content.clear();
                break;
            case Verdict::Blocked:
                row.blocked = true;
                row.pendingUrl = next;
                row.content.clear();
                break;
            case Verdict::Refuse:
                row.refused = true;
                row.content.clear();
                break;
            }
            row.loading = false;
            ++row.serial;
        }
        rowChanged(index);
        emit countChanged();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        showContent(row, messagePage(tr("Page indisponible : %1").arg(reply->errorString())), QUrl());
        rowChanged(index);
        return;
    }
    const QByteArray bytes = reply->readAll();
    const QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
    const QUrl base = reply->url();
    QString html;
    if (type.startsWith(QLatin1String("image/"))) {
        html = QStringLiteral("<img src=\"%1\" style=\"max-width:100%\">")
                   .arg(QString::fromLatin1(base.toEncoded()).toHtmlEscaped());
    } else {
        QStringDecoder decoder = QStringDecoder::decoderForHtml(bytes);
        static const QRegularExpression charsetRe(QStringLiteral(R"(charset\s*=\s*"?([^";\s]+))"));
        const QRegularExpressionMatch charset = charsetRe.match(type);
        if (charset.hasMatch()) {
            if (const auto encoding = QStringConverter::encodingForName(charset.captured(1)))
                decoder = QStringDecoder(*encoding);
        }
        html = decoder.decode(bytes);
        if (!type.isEmpty() && !type.contains(QLatin1String("html")))
            html = QStringLiteral("<pre>%1</pre>").arg(html.toHtmlEscaped());
    }
    showContent(row, html, base);
    rowChanged(index);
}

int WebFrameItems::rowOf(std::uint64_t id) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].id == id)
            return i;
    }
    return -1;
}

void WebFrameItems::rowChanged(int row)
{
    const QModelIndex i = index(row, 0);
    emit dataChanged(i, i);
}

int WebFrameItems::blockedCount() const noexcept
{
    return static_cast<int>(
        std::count_if(m_rows.cbegin(), m_rows.cend(), [](const Row& r) { return r.blocked; }));
}

int WebFrameItems::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant WebFrameItems::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows[index.row()];
    switch (role) {
    case ObjectIdRole:
        return QVariant::fromValue<quint64>(row.id);
    case BoundsRole:
        return row.bounds;
    case HtmlRole:
        return row.html;
    case AddressRole:
        return row.address;
    case BlockedRole:
        return row.blocked;
    case RefusedRole:
        return row.refused;
    case LoadingRole:
        return row.loading;
    case ContentRole:
        return row.content;
    case BaseUrlRole:
        return row.baseUrl;
    case TitleRole: {
        const QUrl shown = row.blocked ? row.pendingUrl : row.baseUrl;
        return shown.host().isEmpty() ? tr("Cadre web") : shown.host();
    }
    case CanGoBackRole:
        return row.historyIndex > 0;
    case CanGoForwardRole:
        return row.historyIndex >= 0 && row.historyIndex + 1 < row.history.size();
    case SerialRole:
        return row.serial;
    default:
        return {};
    }
}

QHash<int, QByteArray> WebFrameItems::roleNames() const
{
    return {{ObjectIdRole, "objectId"}, {BoundsRole, "bounds"},       {HtmlRole, "html"},
            {AddressRole, "address"},   {BlockedRole, "blocked"},     {RefusedRole, "refused"},
            {LoadingRole, "loading"},   {ContentRole, "content"},     {BaseUrlRole, "baseUrl"},
            {TitleRole, "title"},       {CanGoBackRole, "canGoBack"}, {CanGoForwardRole, "canGoForward"},
            {SerialRole, "serial"}};
}

bool WebFrameItems::insertFrame(const QString& html, const QString& address)
{
    if (!m_document || m_document->isBusy())
        return false;
    const QString code = html.trimmed().isEmpty() ? QString() : html;
    QString typed = address.trimmed();
    if (code.isEmpty() && typed.isEmpty())
        return false;
    if (!typed.isEmpty()) {
        const QUrl url = QUrl::fromUserInput(typed);
        if (!url.isValid())
            return false;
        m_policy.typedByUser(m_key, url);
        typed = url.toString();
    }
    const int page = m_document->currentIndex();
    const QSizeF size = m_document->pageSize();
    const double width = std::min(640.0, size.width() * 0.8);
    const double height = std::min(400.0, size.height() * 0.8);
    core::WebFrame frame;
    frame.bounds = {(size.width() - width) / 2, (size.height() - height) / 2, width, height};
    frame.html = code.toStdString();
    frame.address = typed.toStdString();
    core::PageObject object = frame;
    object.common.layer = core::Layer::Middle;
    m_document->execute(std::make_unique<core::AddObjectCommand>(static_cast<std::size_t>(page), object));
    return true;
}

bool WebFrameItems::resizeFrame(int row, const QRectF& bounds)
{
    if (!m_document || row < 0 || row >= m_rows.size() || bounds.width() < 1 || bounds.height() < 1)
        return false;
    const QRectF old = m_rows[row].bounds;
    if (old == bounds || old.width() <= 0 || old.height() <= 0)
        return false;
    const core::Transform delta = core::compose(
        core::compose(core::translation(-old.x(), -old.y()),
                      core::scaling(bounds.width() / old.width(), bounds.height() / old.height())),
        core::translation(bounds.x(), bounds.y()));
    m_document->execute(std::make_unique<core::TransformObjectsCommand>(
        static_cast<std::size_t>(m_page), std::vector<core::ObjectId>{m_rows[row].id}, delta));
    return true;
}

void WebFrameItems::requestUnblock(int row)
{
    if (row < 0 || row >= m_rows.size() || !m_rows[row].blocked)
        return;
    m_pendingRow = row;
    emit unblockPendingChanged();
}

void WebFrameItems::answer(bool confirmed)
{
    if (m_pendingRow < 0)
        return;
    m_pendingRow = -1;
    emit unblockPendingChanged();
    if (!confirmed)
        return;
    m_policy.unblock(m_key, m_page); // cette page de ce document seulement
    for (int i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];
        if (!row.blocked)
            continue;
        const QUrl url = row.pendingUrl;
        navigate(row, url, false);
        rowChanged(i);
    }
    emit countChanged();
}

bool WebFrameItems::followLink(int row, const QUrl& viewUrl)
{
    if (row < 0 || row >= m_rows.size() || !viewUrl.isValid())
        return false;
    Row& r = m_rows[row];
    const QString fragment = viewUrl.fragment();
    if (fragment.startsWith(kLinkPrefix)) {
        bool ok = false;
        const int n = fragment.mid(kLinkPrefix.size()).toInt(&ok);
        if (!ok || n < 0 || n >= r.links.size())
            return true;
        navigate(r, r.links[n], true);
        rowChanged(row);
        emit countChanged();
        return true;
    }
    const QUrl page = viewUrl.adjusted(QUrl::RemoveFragment);
    const QUrl shown = r.baseUrl.adjusted(QUrl::RemoveFragment);
    if (page.isEmpty() || page == shown || page == QUrl(QStringLiteral("about:blank")))
        return false;        // le contenu donné, ou un renvoi dans la même page
    navigate(r, page, true); // navigation du moteur : rendue à la politique
    rowChanged(row);
    emit countChanged();
    return true;
}

void WebFrameItems::goBack(int row)
{
    if (row < 0 || row >= m_rows.size() || m_rows[row].historyIndex <= 0)
        return;
    Row& r = m_rows[row];
    --r.historyIndex;
    navigate(r, r.history[r.historyIndex], false);
    rowChanged(row);
    emit countChanged();
}

void WebFrameItems::goForward(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    Row& r = m_rows[row];
    if (r.historyIndex < 0 || r.historyIndex + 1 >= r.history.size())
        return;
    ++r.historyIndex;
    navigate(r, r.history[r.historyIndex], false);
    rowChanged(row);
    emit countChanged();
}

void WebFrameItems::reload(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    prepareRow(m_rows[row]);
    rowChanged(row);
    emit countChanged();
}

bool WebFrameItems::snapshot(int row)
{
    if (!m_document || !m_grabber || row < 0 || row >= m_rows.size())
        return false;
    const QRectF zone = m_rows[row].bounds;
    const QImage image = m_grabber(zone);
    if (image.isNull())
        return false;
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
        return false;
    core::ImageObject picture;
    picture.bounds = {zone.x(), zone.y(), zone.width(), zone.height()};
    picture.sourceWidth = image.width();
    picture.sourceHeight = image.height();
    core::PageObject object = picture;
    object.common.layer = core::Layer::Middle;
    object.common.name = tr("Image du cadre web").toStdString();
    m_document->execute(std::make_unique<SnapshotCommand>(static_cast<std::size_t>(m_page), std::move(object),
                                                          std::move(png)));
    return true;
}

void WebFrameItems::useWindowCapture(QQuickItem* pageView)
{
    const QPointer<QQuickItem> view(pageView);
    m_grabber = [view](const QRectF& bounds) -> QImage {
        if (!view || !view->window())
            return {};
        QPointF a;
        QPointF b;
        if (!QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, a),
                                       Q_ARG(QPointF, bounds.topLeft())) ||
            !QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, b),
                                       Q_ARG(QPointF, bounds.bottomRight())))
            return {};
        QQuickWindow* window = view->window();
        QScreen* screen = window->screen();
        if (!screen)
            return {};
        const QRect zone = QRectF(a, b).normalized().toAlignedRect();
        // La fenêtre telle qu'à l'écran : la vue native y est, à la différence de grabToImage.
        return screen->grabWindow(window->winId(), zone.x(), zone.y(), zone.width(), zone.height()).toImage();
    };
}

} // namespace actcraft::ui
