#pragma once

#include <actcraft/core/model.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <QFutureWatcher>
#include <QThread>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace actcraft::tools {

// Outil du lot 5 (L5-T12) : membres de l'interface Tool recopiés de selecttool.h (A1).
class FillTool final : public Tool
{
public:
    explicit FillTool(ToolContext& context);
    ~FillTool() override;
    QString id() const override; // « fill »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    void setColor(core::Argb color);
    // Crochet de test : appelé par le fil de calcul d'un remplissage d'image, à la fin du calcul.
    void setComputeObserver(std::function<void(QThread*)> observer);
    // Nombre de résultats d'image jetés parce que le document avait changé entre-temps.
    int staleImageFillDropped() const noexcept;

private:
    void fillImage(core::ObjectId image, core::PointF point);
    void finishImageFill(QFutureWatcher<std::string>* watcher, std::size_t pageIndex, std::uint64_t revision,
                         core::ObjectId image, const std::string& key);

    ToolContext& m_context;
    core::Argb m_color = 0xFF000000u;
    std::function<void(QThread*)> m_observer;
    std::vector<QFutureWatcher<std::string>*> m_jobs;
    int m_stale = 0;
};

} // namespace actcraft::tools
