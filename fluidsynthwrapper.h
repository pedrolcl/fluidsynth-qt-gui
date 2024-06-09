// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#ifndef FLUIDSYNTHWRAPPER_H
#define FLUIDSYNTHWRAPPER_H

#include <QByteArray>
#include <QObject>

#define BUFFER_SIZE 8192
#ifdef Q_OS_WINDOWS
#include <io.h>
#include <namedpipeapi.h>
#include <windows.h>
#define PipeRead(fd, ptr, size) _read(fd, ptr, size)
#define PipeWrite(fd, str, size) _write(fd, str, size)
#define PipeNew(fds) _pipe(fds, BUFFER_SIZE, _O_BINARY | _O_NOINHERIT)
#define PipeClose(fd) _close(fd)
#define PipeNonBlock(fd) \
    DWORD lpMode = PIPE_READMODE_BYTE | PIPE_NOWAIT; \
    auto hfdPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd)); \
    SetNamedPipeHandleState(hfdPipe, &lpMode, nullptr, nullptr)
#else
#include <unistd.h>
#define PipeRead(fd, ptr, size) read(fd, ptr, size)
#define PipeWrite(fd, str, size) write(fd, str, size)
#define PipeNew(fds) pipe2(fds, O_NONBLOCK)
#define PipeClose(fd) close(fd)
#define PipeNonBlock(fd)
#endif
#include <fcntl.h>

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

    void deinit();
    void createMidiPlayer();

public:
    explicit FluidSynthWrapper(QObject *parent = nullptr);
    ~FluidSynthWrapper() override;

    void init(const QString &audioDriver,
              const QString &midiDriver,
              const QString &configFile,
              const QStringList &args);

    QByteArray prompt() const;

public slots:
    void command(const QByteArray &cmd);
    void readPipe();
    void loadMIDIFiles(const QStringList &fileNames);

signals:
    void readyRead();
    void initialized();
    void diagnostics(int level, const QByteArray message);
    void dataRead(const QByteArray &data, const int res);
};

#endif // FLUIDSYNTHWRAPPER_H
