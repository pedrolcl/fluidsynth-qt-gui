// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#include <QStringListModel>

#include "fluidcompleter.h"

FluidCompleter::FluidCompleter(QObject *parent)
    : ConsoleWidgetCompleter(parent)
{
    m_keywords = {"basicchannels",
                  "breathmode",
                  "cc",
                  "channels",
                  "channelsmode",
                  "chorus",
                  "cho_set_depth",
                  "cho_set_level",
                  "cho_set_nr",
                  "cho_set_speed",
                  "dumptuning",
                  "echo",
                  "fonts",
                  "gain",
                  "get",
                  "help",
                  "info",
                  "inst",
                  "interp",
                  "interpc",
                  "ladspa_buffer",
                  "ladspa_check",
                  "ladspa_effect",
                  "ladspa_link",
                  "ladspa_reset",
                  "ladspa_set",
                  "ladspa_start",
                  "ladspa_stop",
                  "legatomode",
                  "load",
                  "noteoff",
                  "noteon",
                  "pitch_bend",
                  "pitch_bend_range",
                  "player_cont",
                  "player_loop",
                  "player_next",
                  "player_seek",
                  "player_start",
                  "player_stop",
                  "player_tempo_bpm",
                  "player_tempo_int",
                  "portamentomode",
                  "prog",
                  "quit",
                  "reload",
                  "reset",
                  "resetbasicchannels",
                  "resettuning",
                  "reverb",
                  "rev_preset",
                  "rev_setdamp",
                  "rev_setlevel",
                  "rev_setroomsize",
                  "rev_setwidth",
                  "router_begin",
                  "router_chan",
                  "router_clear",
                  "router_default",
                  "router_end",
                  "router_par1",
                  "router_par2",
                  "select",
                  "set",
                  "setbasicchannels",
                  "setbreathmode",
                  "setlegatomode",
                  "setportamentomode",
                  "settings",
                  "settuning",
                  "sleep",
                  "source",
                  "tune",
                  "tuning",
                  "tunings",
                  "unload",
                  "voice_count"};
    m_keywords.removeDuplicates();
    m_keywords.sort(Qt::CaseInsensitive);
}

int FluidCompleter::updateCompletionModel(const QString &code)
{
    // Start by clearing the model
    this->setModel(0);

    // Don't try to complete the empty string
    if (code.isEmpty()) {
        return 0;
    }

    // Search backward through the string for usable characters
    QString textToComplete;
    for (int i = code.length() - 1; i >= 0; i--) {
        QChar c = code.at(i);
        if (c.isLetterOrNumber() || c == '.' || c == '_') {
            textToComplete.prepend(c);
            m_insert_pos = i;
        } else {
            break;
        }
    }

    // Split the string at the last dot, if one exists
    QString lookup;
    QString compareText = textToComplete;
    int dot = compareText.lastIndexOf('.');
    if (dot != -1) {
        lookup = compareText.mid(0, dot);
        compareText = compareText.mid(dot + 1);
        m_insert_pos += (dot + 1);
    }

    // Lookup QtScript names
    QStringList found;
    if (!lookup.isEmpty() || !compareText.isEmpty()) {
        compareText = compareText.toLower();
        QStringList l = m_keywords;
        foreach (QString n, l)
            if (n.toLower().startsWith(compareText))
                found << n;
    }

    // Initialize the completion model
    if (!found.isEmpty()) {
        setCompletionMode(QCompleter::PopupCompletion);
        setModel(new QStringListModel(found, this));
        setCaseSensitivity(Qt::CaseInsensitive);
        setCompletionPrefix(compareText.toLower());
    }

    return found.size();
}
