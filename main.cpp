#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile logFile("debug.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " ";
        switch (type) {
            case QtDebugMsg: stream << "Debug: "; break;
            case QtInfoMsg: stream << "Info: "; break;
            case QtWarningMsg: stream << "Warning: "; break;
            case QtCriticalMsg: stream << "Critical: "; break;
            case QtFatalMsg: stream << "Fatal: "; break;
        }
        stream << msg << "\n";
        logFile.close();
    }
}

int main(int argc, char *argv[])
{
    try {
        // 安装消息处理器
        qInstallMessageHandler(messageHandler);
        qDebug() << "Application starting...";

        QApplication a(argc, argv);
        qDebug() << "QApplication created";

        MainWindow w;
        qDebug() << "MainWindow created";
        
        w.show();
        qDebug() << "MainWindow shown";

        return a.exec();
    } catch (const std::exception& e) {
        QFile logFile("debug.log");
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream stream(&logFile);
            stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") 
                   << " Fatal Error: " << e.what() << "\n";
            logFile.close();
        }
        return 1;
    }
}
