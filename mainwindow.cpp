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
    m_client = new FluidSynthWrapper(audioDriver, midiDriver, configFile, args, this);
    m_completer = new FluidCompleter(this);
    m_console = new ConsoleWidget(this);
    m_console->setCompleter(m_completer);
    m_console->setFont(QFont("Monospace"));
    m_console->writeStdOut("Type 'help' for help topics.\n");
    setCentralWidget(m_console);
    setWindowTitle("FluidSynth Commmand Window");
    QMenu *file = menuBar()->addMenu("&File");
    file->addAction("E&xit", QKeySequence::Quit, this, &MainWindow::close);
    connect(m_client, &FluidSynthWrapper::dataRead, this, &MainWindow::consoleOutput);
    connect(m_console->device(), &QIODevice::readyRead, this, &MainWindow::consoleInput);
    consoleOutput(m_client->prompt());
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
