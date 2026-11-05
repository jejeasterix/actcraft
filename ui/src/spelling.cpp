#include <actcraft/ui/spelling.h>

namespace actcraft::ui {

Spelling::Spelling(QObject* parent) : QObject(parent) {}
Spelling::~Spelling() = default;

void Spelling::setDocument(DocumentController*) {}
void Spelling::setProfile(ProfileController*) {}
QString Spelling::userDictionaryPath() const { return m_userDictionaryPath; }
void Spelling::setUserDictionaryPath(const QString& path) { m_userDictionaryPath = path; }
QString Spelling::dictionaryFolder() const { return m_dictionaryFolder; }
void Spelling::setDictionaryFolder(const QString& folder) { m_dictionaryFolder = folder; }
void Spelling::setContactActive(bool) {}
QString Spelling::word() const { return {}; }
void Spelling::start() {}
void Spelling::ignore() {}
void Spelling::ignoreAlways() {}
void Spelling::addToDictionary() {}
void Spelling::replace(const QString&) {}
void Spelling::replaceAll(const QString&) {}
void Spelling::close() {}
bool Spelling::prepare() { return false; }
void Spelling::advance() {}
void Spelling::publish(std::optional<core::SpellHit>) {}
void Spelling::showPendingPage() {}
QRectF Spelling::highlightOf(const core::SpellHit&) const { return {}; }
bool Spelling::replaceCurrent(const QString&) { return false; }

} // namespace actcraft::ui
