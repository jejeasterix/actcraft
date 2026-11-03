#pragma once

#include <QFuture>
#include <QPromise>
#include <QString>
#include <QThread>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

// Aides privées du module d'export, partagées par l'export de page, l'import de PDF et la conversion de
// présentation : chacun rend un résultat doté d'un champ `error` et le calcule hors du fil principal.
namespace actcraft::exporting::detail {

// Un fil à nous, pas le groupe de fils : en attendant le résultat, QFuture::result() peut exécuter la tâche
// encore en file sur le fil qui attend, donc sur le fil principal.
template <class Work> auto runAsync(Work work) -> QFuture<std::invoke_result_t<Work&>>
{
    using Result = std::invoke_result_t<Work&>;
    auto promise = std::make_shared<QPromise<Result>>();
    promise->start();
    QFuture<Result> future = promise->future();
    QThread* thread = QThread::create([promise, task = std::move(work)]() mutable {
        promise->addResult(task());
        promise->finish();
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return future;
}

template <class Result> Result failure(const QString& error)
{
    Result result;
    result.error = error;
    return result;
}

} // namespace actcraft::exporting::detail
