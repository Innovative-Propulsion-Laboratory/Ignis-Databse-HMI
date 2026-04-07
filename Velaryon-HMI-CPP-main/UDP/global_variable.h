#ifndef GLOBAL_VARIABLE_H
#define GLOBAL_VARIABLE_H

#include <QTableWidget>
#include <QPlainTextEdit>
#include <QDateTime>


extern bool ack;                            // Acknowledgement flag for received messages
extern bool launch_test;                    // Launch test flag
extern int countdown;                       // Countdown value (time remaining before launch)
extern bool end_test;                       // End test flag
extern QPlainTextEdit* globalLogTerminal;   // Global log terminal (shared UI widget for displaying log messages)
extern QString timestamp;                   // Timestamp for filename
extern QString filepath_log;                // Filepath for log text file
extern QString filepath_csv;                // Filepath for standard CSV logs
extern QString filepath_csv_HF;             // Filepath for high-frequency CSV logs


#endif // GLOBAL_VARIABLE_H
