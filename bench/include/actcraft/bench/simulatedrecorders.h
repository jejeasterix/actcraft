#pragma once

#include <actcraft/ui/recorders.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QTimer>

#include <functional>
#include <memory>

namespace actcraft::bench {

// Moteur de son simulé pour le banc et les captures hors écran (aucun micro, aucune fenêtre de permission) :
// un WAV silencieux de la durée enregistrée, pauses exclues, écrit à la fermeture. Le code livré ne le
// connaît pas : c'est le banc qui le pose, par Recorders::setDefaultEngines, avant de charger l'interface.
class SilentSoundEngine final : public media::RecordingEngine
{
public:
    void start(const QString& path) override
    {
        m_path = path;
        m_clock.start();
    }
    void pause() override
    {
        m_recorded += m_clock.elapsed();
        m_running = false;
    }
    void resume() override
    {
        m_clock.restart();
        m_running = true;
    }
    void stop() override
    {
        if (m_running)
            m_recorded += m_clock.elapsed();
        m_running = false;
        const QString path = m_path;
        const quint32 samples = static_cast<quint32>(m_recorded * 8); // 8 kHz
        QByteArray wav("RIFF");
        putLe(wav, 36 + samples * 2, 4);
        wav += "WAVEfmt ";
        putLe(wav, 16, 4);
        putLe(wav, 1, 2);
        putLe(wav, 1, 2);
        putLe(wav, 8000, 4);
        putLe(wav, 16000, 4);
        putLe(wav, 2, 2);
        putLe(wav, 16, 2);
        wav += "data";
        putLe(wav, samples * 2, 4);
        wav += QByteArray(static_cast<qsizetype>(samples) * 2, '\0');
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(wav) != wav.size()) {
            QTimer::singleShot(
                0, this, [this] { emit failed(QStringLiteral("Fichier d'enregistrement illisible.")); });
            return;
        }
        file.close();
        QTimer::singleShot(0, this, [this, path] { emit stopped(path); });
    }
    qint64 duration() const override { return m_recorded + (m_running ? m_clock.elapsed() : 0); }

private:
    static void putLe(QByteArray& out, quint32 value, int bytes)
    {
        for (int i = 0; i < bytes; ++i)
            out.append(static_cast<char>((value >> (8 * i)) & 0xFF));
    }

    QString m_path;
    QElapsedTimer m_clock;
    qint64 m_recorded = 0;
    bool m_running = true;
};

// Plateforme sans micro (hors écran) : le son passe par le moteur muet et la permission est accordée. L'écran
// garde l'enregistreur de la plateforme (absent hors Windows).
inline void useSimulatedRecorders()
{
    ui::Recorders::setDefaultEngines(
        {[] { return std::unique_ptr<media::RecordingEngine>(new SilentSoundEngine); },
         [](std::function<void(bool)> answer) { answer(true); },
         {}});
}

} // namespace actcraft::bench
