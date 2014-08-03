#include <QCoreApplication>
#include <QImage>
#include <QFile>

#include "encoder.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QImage img("source.png");
    QFile configFile("config");
    configFile.open(QIODevice::ReadOnly);
    QString config = QString::fromUtf8(configFile.readAll());
    Encoder encoder(img.size(), QString("ttt.mkv"), config);

    for(int i=0;i<10;++i) {
        encoder.onImage(img);
    }
    encoder.finish();

//    return a.exec();
    return 0;
}
