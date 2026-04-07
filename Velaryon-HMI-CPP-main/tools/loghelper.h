#ifndef LOGHELPER_H
#define LOGHELPER_H

#include <QPlainTextEdit>
#include <QDateEdit>
#include <QFile>
#include "UDP/global_variable.h"


// Log message in log terminal and register it in the log file
inline void logMessage(QPlainTextEdit* terminal, const QString& message) {

    // Guard : terminal peut être nullptr si un message UDP arrive avant
    // que le TestTab (et donc globalLogTerminal) ne soit initialisé
    if (!terminal) return;

    // Default format
    QTextCharFormat format;
    format.setFontWeight(QFont::Normal);
    format.setForeground(QBrush(Qt::white));

    // If warning, color is orange, if error, color's red
    if (message.startsWith("warning", Qt::CaseInsensitive)) {
        format.setFontWeight(QFont::Bold);
        format.setForeground(QBrush(QColor("orange")));
    } else if (message.startsWith("error", Qt::CaseInsensitive)) {
        format.setFontWeight(QFont::Bold);
        format.setForeground(QBrush(Qt::red));
    }

    // Add time
    QString date = QDateTime::currentDateTime().toString("hh:mm:ss");
    QTextCursor cursor = terminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("[" + date + "] " + message + "\n", format);
    terminal->setTextCursor(cursor);

    // Register in file
    QFile file(filepath_log);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << message << "\n";
    }
}

#endif // LOGHELPER_H
