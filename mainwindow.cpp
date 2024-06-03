// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#include <QDebug>
#include <QMenu>
#include <QMenuBar>

#include "ConsoleWidget.h"
#include "fluidcompleter.h"
#include "fluidsynthwrapper.h"
#include "mainwindow.h"

MainWindow::MainWindow(const QString &audioDriver,
                       const QString &midiDriver,
                       const QString &configFile,
                       const QStringList &args,
                       QWidget *parent)
    : QMainWindow{parent}
{
    m_client = new FluidSynthWrapper(this);
    m_completer = new FluidCompleter(this);
    m_console = new ConsoleWidget(this);
    m_console->setCompleter(m_completer);
    m_console->setFont(QFont("Monospace"));
    connect(m_client, &FluidSynthWrapper::dataRead, this, &MainWindow::consoleOutput);
    connect(m_client, &FluidSynthWrapper::diagnostics, this, &MainWindow::diagnosticsOutput);
    connect(m_client, &FluidSynthWrapper::initialized, this, &MainWindow::startInput);
    connect(m_console->device(), &QIODevice::readyRead, this, &MainWindow::consoleInput);

    QMenu *file = menuBar()->addMenu("&File");
    file->addAction("E&xit", QKeySequence::Quit, this, &MainWindow::close);
    setWindowTitle("FluidSynth Commmand Window");
    setCentralWidget(m_console);

    m_client->init(audioDriver, midiDriver, configFile, args);
}

void MainWindow::consoleOutput(const QByteArray &data, const int res)
{
    if (res == 0) {
        m_console->writeStdOut(QString::fromUtf8(data));
    } else {
        m_console->writeStdErr(QString::fromUtf8(data));
    }
    m_console->setMode(ConsoleWidget::Input);
}

void MainWindow::diagnosticsOutput(int level, const QByteArray message)
{
    static const QMap<int, QByteArray> prefix{{fluid_log_level::FLUID_ERR, "Error"},
                                              {fluid_log_level::FLUID_WARN, "Warning"},
                                              {fluid_log_level::FLUID_INFO, "Information"},
                                              {fluid_log_level::FLUID_DBG, "Debug"}};
    QByteArray buffer(prefix[level]);
    buffer.append(": ");
    buffer.append(message);
    buffer.append("\n");
    if (level < fluid_log_level::FLUID_INFO) {
        m_console->writeStdErr(QString::fromUtf8(buffer));
    } else {
        m_console->writeStdOut(QString::fromUtf8(buffer));
    }
}

void MainWindow::consoleInput()
{
    QByteArray text = m_console->device()->readAll();
    if (!text.isEmpty()) {
        m_client->command(text);
    }
    if (text == "quit\n") {
        close();
    } else {
        consoleOutput(m_client->prompt());
    }
}

void MainWindow::startInput()
{
    m_console->writeStdOut("Type 'help' for help topics.\n");
    consoleOutput(m_client->prompt());
}
