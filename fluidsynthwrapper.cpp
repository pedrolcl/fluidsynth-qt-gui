// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#include <QDebug>
#include <QFileInfo>
#include <QTimer>

#include "fluidsynthwrapper.h"

static void FluidSynthWrapper_log_function(int level, const char *message, void *data)
{
    FluidSynthWrapper *classInstance = static_cast<FluidSynthWrapper *>(data);
    emit classInstance->diagnostics(level, QByteArray(message));
}

void FluidSynthWrapper::init(const QString &audioDriver,
                             const QString &midiDriver,
                             const QString &configFile,
                             const QStringList &args)
{
    //fluid_set_log_function(fluid_log_level::FLUID_DBG, &FluidSynthWrapper_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_ERR, &FluidSynthWrapper_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_WARN, &FluidSynthWrapper_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_INFO, &FluidSynthWrapper_log_function, this);

    m_settings = new_fluid_settings();
    fluid_settings_setint(m_settings, "midi.autoconnect", 1);
    fluid_settings_setstr(m_settings, "shell.prompt", "> ");

    QFileInfo fileInfo(configFile);
    QByteArray fileName_utf8;
    fileName_utf8.reserve(BUFFER_SIZE);
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
            fluid_log(FLUID_WARN,
                      "Failed to execute command configuration file %s",
                      fileInfo.absoluteFilePath().toUtf8().data());
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
        fluid_log(FLUID_WARN, "Failed to create the synthesizer");
        return;
    }

    foreach (const auto fileName, args) {
        fileName_utf8 = fileName.toUtf8();
        if (fluid_is_midifile(fileName_utf8.data())) {
            continue;
        }
        if (fluid_is_soundfont(fileName_utf8.data())) {
            if (fluid_synth_sfload(m_synth, fileName_utf8.data(), 1) == -1) {
                fluid_log(FLUID_WARN, "Failed to load the SoundFont %s", fileName.toUtf8().data());
            }
        } else {
            fluid_log(
                FLUID_WARN,
                "Parameter %s is not a SoundFont or MIDI file or error occurred identifying it.",
                fileName.toUtf8().data());
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
        fluid_log(FLUID_WARN,
                  "Failed to create the MIDI input router; no MIDI input\n"
                  "will be available. You can access the synthesizer \n"
                  "through the console.");
    }

    /* start the midi router and link it to the synth */
    if (m_router != nullptr) {
        m_midi_driver = new_fluid_midi_driver(m_settings,
                                              fluid_midi_router_handle_midi_event,
                                              (void *) m_router);

        if (m_midi_driver == nullptr) {
            fluid_log(FLUID_WARN,
                      "Failed to create the MIDI thread; no MIDI input\n"
                      "will be available. You can access the synthesizer \n"
                      "through the console.");
        }
    }

    /* create the player and add any midi files, if requested */
    createMidiPlayer();
    QStringList midiFiles;
    foreach (const auto fileName, args) {
        QByteArray file = fileName.toUtf8();
        if (fluid_is_midifile(file.data())) {
            midiFiles.append(fileName);
        }
    }
    if (!midiFiles.isEmpty()) {
        loadMIDIFiles(midiFiles);
    }

    m_cmd_handler = new_fluid_cmd_handler2(m_settings, m_synth, m_router, m_player);
    if (m_cmd_handler == nullptr) {
        fluid_log(FLUID_WARN, "Failed to create the command handler");
        return;
    }

    m_audio_driver = new_fluid_audio_driver(m_settings, m_synth);
    if (m_audio_driver == nullptr) {
        fluid_log(FLUID_WARN, "Failed to create the audio driver. Giving up.");
        return;
    }
    
    fluid_log(FLUID_INFO, "FluidSynth runtime version %s", fluid_version_str());
    
    QTimer::singleShot(100, this, &FluidSynthWrapper::initialized);
}

void FluidSynthWrapper::deinit()
{
    //fluid_set_log_function(fluid_log_level::FLUID_DBG, fluid_default_log_function, nullptr);
    fluid_set_log_function(fluid_log_level::FLUID_ERR, fluid_default_log_function, nullptr);
    fluid_set_log_function(fluid_log_level::FLUID_WARN, fluid_default_log_function, nullptr);
    fluid_set_log_function(fluid_log_level::FLUID_INFO, fluid_default_log_function, nullptr);
    
    delete_fluid_cmd_handler(m_cmd_handler);

    destroyMidiPlayer();
    delete_fluid_audio_driver(m_audio_driver);
    delete_fluid_midi_driver(m_midi_driver);
    delete_fluid_midi_router(m_router);
    delete_fluid_synth(m_synth);
    delete_fluid_settings(m_settings);
}

FluidSynthWrapper::FluidSynthWrapper(QObject *parent)
    : QObject{parent}
{
    auto res = PipeNew(m_pipefds);
    Q_ASSERT_X(res == 0, "FluidSynthWrapper", "Error creating the pipe");
    PipeNonBlock(m_pipefds[FDREAD]);
    connect(this,
            &FluidSynthWrapper::readyRead,
            this,
            &FluidSynthWrapper::readPipe);
}

FluidSynthWrapper::~FluidSynthWrapper()
{
    PipeClose(m_pipefds[FDREAD]);
    PipeClose(m_pipefds[FDWRITE]);
    deinit();
}

QByteArray FluidSynthWrapper::prompt() const
{
    if (m_settings) {
        char *s;
        int res = fluid_settings_dupstr(m_settings, "shell.prompt", &s);
        if (res == FLUID_OK) {
            QByteArray p(s);
            fluid_free(s);
            return p;
        }
    }
    return "> ";
}

void FluidSynthWrapper::command(const QByteArray &cmd)
{
    if (m_cmd_handler && !cmd.isEmpty() && cmd != "\n") {
        m_cmdresult = fluid_command(m_cmd_handler, cmd.data(), m_pipefds[FDWRITE]);
        QTimer::singleShot(50, this, &FluidSynthWrapper::readyRead);
    }
}

void FluidSynthWrapper::readPipe()
{
    QByteArray buffer;
    buffer.reserve(BUFFER_SIZE);
    qsizetype readBytes = PipeRead(m_pipefds[FDREAD], buffer.data(), buffer.capacity());
    //qDebug() << Q_FUNC_INFO << readBytes;
    if (readBytes > 0) {
        buffer.resize(readBytes);
        emit dataRead(buffer, m_cmdresult);
    }
}

void FluidSynthWrapper::createMidiPlayer()
{
    m_player = new_fluid_player(m_synth);
    if (m_player == nullptr) {
        fluid_log(FLUID_WARN,
                  "Failed to create the midifile player.\n"
                  "Continuing without a player.");
    } else if (m_router != nullptr) {
        fluid_player_set_playback_callback(m_player, fluid_midi_router_handle_midi_event, m_router);
    }
}

void FluidSynthWrapper::destroyMidiPlayer()
{
    if (m_player != nullptr) {
        fluid_player_stop(m_player);
        fluid_player_join(m_player);
    }
    if (fluid_player_get_status(m_player) == FLUID_PLAYER_DONE) {
        delete_fluid_player(m_player);
        m_player = nullptr;
    }
}

void FluidSynthWrapper::loadMIDIFiles(const QStringList &fileNames)
{
    destroyMidiPlayer();
    if (m_player == nullptr) {
        createMidiPlayer();
    }
    foreach (const auto fileName, fileNames) {
        QByteArray f = fileName.toUtf8();
        if (m_player) {
            if (fluid_player_add(m_player, f.data()) == FLUID_FAILED) {
                fluid_log(FLUID_WARN, "file cannot be played: %s", f.data());
            }
        }
    }
    if (!fileNames.isEmpty()) {
        fluid_player_play(m_player);
        emit midiPlayerActive();
    }
}
