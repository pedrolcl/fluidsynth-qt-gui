// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#include "fluidsynthwrapper.h"
#include <QDebug>
#include <QFileInfo>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

void FluidSynthWrapper::init(const QString &audioDriver,
                             const QString &midiDriver,
                             const QString &configFile,
                             const QStringList &args)
{
    m_settings = new_fluid_settings();
    fluid_settings_setint(m_settings, "midi.autoconnect", 1);
    fluid_settings_setstr(m_settings, "shell.prompt", "> ");

    QFileInfo fileInfo(configFile);
    QByteArray fileName_utf8;
    fileName_utf8.reserve(4096);
    char *config_file = nullptr;

    if (!configFile.isEmpty() && fileInfo.exists()) {
        fileName_utf8 = configFile.toUtf8();
        config_file = fileName_utf8.data();
    }

    if (config_file == nullptr || !fileInfo.exists()) {
        config_file = fluid_get_userconf(fileName_utf8.data(), fileName_utf8.capacity());
        fileInfo.setFile(config_file);
        if (config_file == nullptr || !fileInfo.exists()) {
            config_file = fluid_get_sysconf(fileName_utf8.data(), fileName_utf8.capacity());
            if (config_file != nullptr) {
                fileInfo.setFile(config_file);
                if (!fileInfo.exists()) {
                    config_file = nullptr;
                }
            }
        }
    }

    if (config_file != nullptr && fileInfo.exists()) {
        m_cmd_handler = new_fluid_cmd_handler2(m_settings, nullptr, nullptr, nullptr);
        auto res = fluid_source(m_cmd_handler, config_file);
        if (res < 0) {
            qWarning() << "Failed to execute command configuration file"
                       << fileInfo.absoluteFilePath();
        }
        delete_fluid_cmd_handler(m_cmd_handler);
        m_cmd_handler = nullptr;
    }

    if (!audioDriver.isNull()) {
        const QByteArray audioDriver_utf8 = audioDriver.toUtf8();
        if (fluid_settings_setstr(m_settings, "audio.driver", audioDriver_utf8.data()) != FLUID_OK) {
            return;
        }
    }

    if (!midiDriver.isNull()) {
        const QByteArray midiDriver_utf8 = midiDriver.toUtf8();
        if (fluid_settings_setstr(m_settings, "midi.driver", midiDriver_utf8.data()) != FLUID_OK) {
            return;
        }
    }

    m_synth = new_fluid_synth(m_settings);
    if (m_synth == nullptr) {
        qWarning() << "Failed to create the synthesizer";
        return;
    }

    foreach (const auto fileName, args) {
        fileName_utf8 = fileName.toUtf8();
        if (fluid_is_midifile(fileName_utf8.data())) {
            continue;
        }
        if (fluid_is_soundfont(fileName_utf8.data())) {
            if (fluid_synth_sfload(m_synth, fileName_utf8.data(), 1) == -1) {
                qWarning() << "Failed to load the SoundFont" << fileName;
            }
        } else {
            qWarning() << "Parameter" << fileName
                       << "is not a SoundFont or MIDI file or error occurred identifying it.";
        }
    }

    /* Try to load the default soundfont, if no soundfont specified */
    if (fluid_synth_get_sfont(m_synth, 0) == nullptr) {
        char *s;
        if (fluid_settings_dupstr(m_settings, "synth.default-soundfont", &s) != FLUID_OK) {
            s = nullptr;
        }
        if ((s != nullptr) && (s[0] != '\0')) {
            fluid_synth_sfload(m_synth, s, 1);
        }
        fluid_free(s);
    }

    m_router = new_fluid_midi_router(m_settings, fluid_synth_handle_midi_event, (void *) m_synth);
    if (m_router == nullptr) {
        qWarning() << "Failed to create the MIDI input router; no MIDI input\n"
                      "will be available. You can access the synthesizer \n"
                      "through the console.";
    }

    /* start the midi router and link it to the synth */
    if (m_router != nullptr) {
        m_midi_driver = new_fluid_midi_driver(m_settings,
                                              fluid_midi_router_handle_midi_event,
                                              (void *) m_router);

        if (m_midi_driver == nullptr) {
            qWarning() << "Failed to create the MIDI thread; no MIDI input\n"
                          "will be available. You can access the synthesizer \n"
                          "through the console.";
        }
    }

    /* create the player and add any midi files, if requested */
    foreach (const auto fileName, args) {
        const QByteArray file_utf8 = fileName.toUtf8();
        const char *file = file_utf8.data();
        if (fluid_is_midifile(file)) {
            if (m_player == nullptr) {
                m_player = new_fluid_player(m_synth);

                if (m_player == nullptr) {
                    qWarning() << "Failed to create the midifile player.\n"
                                  "Continuing without a player.";
                    break;
                }

                if (m_router != nullptr) {
                    fluid_player_set_playback_callback(m_player,
                                                       fluid_midi_router_handle_midi_event,
                                                       m_router);
                }
            }
            fluid_player_add(m_player, file);
        }
    }

    m_cmd_handler = new_fluid_cmd_handler2(m_settings, m_synth, m_router, m_player);
    if (m_cmd_handler == nullptr) {
        qWarning() << "Failed to create the command handler";
        return;
    }

    if (m_player != nullptr) {
        fluid_player_play(m_player);
    }

    m_audio_driver = new_fluid_audio_driver(m_settings, m_synth);
    if (m_audio_driver == nullptr) {
        qWarning() << "Failed to create the audio driver. Giving up.";
        return;
    }
}

void FluidSynthWrapper::deinit()
{
    delete_fluid_cmd_handler(m_cmd_handler);

    if (m_player) {
        fluid_player_stop(m_player);
        if (m_audio_driver != nullptr
            || !fluid_settings_str_equal(m_settings, "player.timing-source", "sample")) {
            /* if no audio driver and sample timers are used, nothing makes the player advance */
            fluid_player_join(m_player);
        }
    }
    delete_fluid_audio_driver(m_audio_driver);
    delete_fluid_player(m_player);
    delete_fluid_midi_driver(m_midi_driver);
    delete_fluid_midi_router(m_router);
    delete_fluid_synth(m_synth);
    delete_fluid_settings(m_settings);
}

FluidSynthWrapper::FluidSynthWrapper(const QString &audioDriver,
                                     const QString &midiDriver,
                                     const QString &configFile,
                                     const QStringList &args,
                                     QObject *parent)
    : QObject{parent}
{
    init(audioDriver, midiDriver, configFile, args);
    if (::pipe2(m_pipefds, O_NONBLOCK) == 0) {
        m_notifier = new QSocketNotifier(m_pipefds[FDREAD], QSocketNotifier::Read, this);
        connect(m_notifier,
                &QSocketNotifier::activated,
                this,
                &FluidSynthWrapper::notifierActivated);
    }
}

FluidSynthWrapper::~FluidSynthWrapper()
{
    ::close(m_pipefds[FDREAD]);
    ::close(m_pipefds[FDWRITE]);
    deinit();
}

QByteArray FluidSynthWrapper::prompt() const
{
    if (m_settings) {
        char *s;
        int res = fluid_settings_dupstr(m_settings, "shell.prompt", &s);
        if (res == FLUID_OK) {
            return QByteArray(s);
            fluid_free(s);
        }
    }
    return "> ";
}

void FluidSynthWrapper::command(const QByteArray &cmd)
{
    if (m_cmd_handler && !cmd.isEmpty() && cmd != "\n") {
        m_cmdresult = fluid_command(m_cmd_handler, cmd.data(), m_pipefds[FDWRITE]);
    }
}

void FluidSynthWrapper::notifierActivated(QSocketDescriptor d, QSocketNotifier::Type t)
{
    QByteArray buffer;
    buffer.reserve(4096);
    qsizetype readBytes = ::read(m_pipefds[FDREAD], buffer.data(), buffer.capacity());
    buffer.resize(readBytes);
    if (!buffer.isEmpty()) {
        emit dataRead(buffer, m_cmdresult);
    }
}
