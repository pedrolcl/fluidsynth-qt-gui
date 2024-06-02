// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#ifndef FLUIDSYNTHWRAPPER_H
#define FLUIDSYNTHWRAPPER_H

#include <QByteArray>
#include <QObject>
#include <QSocketNotifier>

#include <fluidsynth.h>

#define FDNULL -1
#define FDREAD 0
#define FDWRITE 1

class FluidSynthWrapper : public QObject
{
    Q_OBJECT

    fluid_settings_t *m_settings{nullptr};
    fluid_player_t *m_player{nullptr};
    fluid_midi_router_t *m_router{nullptr};
    fluid_midi_driver_t *m_midi_driver{nullptr};
    fluid_audio_driver_t *m_audio_driver{nullptr};
    fluid_synth_t *m_synth{nullptr};
    fluid_cmd_handler_t *m_cmd_handler{nullptr};
    int m_cmdresult{0};

    int m_pipefds[2]{FDNULL, FDNULL};
    QSocketNotifier *m_notifier{nullptr};

    void init(const QString &audioDriver,
              const QString &midiDriver,
              const QString &configFile,
              const QStringList &args);
    void deinit();

public:
    explicit FluidSynthWrapper(const QString &audioDriver,
                               const QString &midiDriver,
                               const QString &configFile,
                               const QStringList &args,
                               QObject *parent = nullptr);
    ~FluidSynthWrapper() override;

    QByteArray prompt() const;

public slots:
    void command(const QByteArray &cmd);
    void notifierActivated(QSocketDescriptor d, QSocketNotifier::Type t);

signals:
    void dataRead(const QByteArray &data, const int res);
};

#endif // FLUIDSYNTHWRAPPER_H
