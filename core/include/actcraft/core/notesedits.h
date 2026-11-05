#pragma once

#include <actcraft/core/undo.h>

#include <chrono>
#include <string>

namespace actcraft::core {

inline constexpr int kMergeNotes = 7; // famille de fusion de SetPageNotesCommand (une frappe continue)

// Notes d'une page (F-830, HTML). Un changement de page qui n'atteint pas le cache (PageSettingsChanged) :
// la frappe continue (même page, moins de kMergeWindow entre deux frappes) se fusionne en une étape.
class SetPageNotesCommand final : public UndoCommand
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds kMergeWindow{2000};

    SetPageNotesCommand(int page, std::string html, Clock::time_point when = Clock::now());
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    int mergeId() const noexcept override;
    bool mergeWith(const UndoCommand& other) override;

private:
    int m_page;
    std::string m_html;
    std::string m_old;
    Clock::time_point m_when;
};

} // namespace actcraft::core
