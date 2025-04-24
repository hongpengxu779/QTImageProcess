#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

// 全局变量保存日志文件名
QString logFileName;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 处理要写入的消息
    QString formattedMessage = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + " ";
    
    switch (type) {
        case QtDebugMsg: formattedMessage += "Debug: "; break;
        case QtInfoMsg: formattedMessage += "Info: "; break;
        case QtWarningMsg: formattedMessage += "Warning: "; break;
        case QtCriticalMsg: formattedMessage += "Critical: "; break;
        case QtFatalMsg: formattedMessage += "Fatal: "; break;
    }
    
    formattedMessage += msg;

    // 输出到控制台
    fprintf(stderr, "%s\n", formattedMessage.toLocal8Bit().constData());
    
    // 输出到日志文件
    QFile logFile(logFileName);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << formattedMessage << "\n";
        logFile.close();
    }
}

int main(int argc, char *argv[])
{
    try {
        // 创建一个包含当前日期时间的日志文件名
        logFileName = "debug_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";
        
        // 安装消息处理器
        qInstallMessageHandler(messageHandler);
        qDebug() << "Application starting...";
        qDebug() << "Log file: " << logFileName;

        QApplication a(argc, argv);
        qDebug() << "QApplication created";

        MainWindow w;
        qDebug() << "MainWindow created";
        
        w.show();
        qDebug() << "MainWindow shown";

        return a.exec();
    } catch (const std::exception& e) {
        // 如果在创建日志文件名之前发生异常，创建一个默认日志文件名
        if (logFileName.isEmpty()) {
            logFileName = "debug_error.log";
        }
        
        QFile logFile(logFileName);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream stream(&logFile);
            stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") 
                   << " Fatal Error: " << e.what() << "\n";
            logFile.close();
        }
        
        // 也输出到控制台
        fprintf(stderr, "Fatal Error: %s\n", e.what());
        
        return 1;
    }
}
