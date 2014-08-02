#include <QCoreApplication>
#include <QImage>

#include "encoder.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QImage img("source.png");
    Encoder encoder(img.size(), QString("ttt.mkv"));

    for(int i=0;i<10;++i) {
        encoder.onImage(img);
    }
    encoder.finish();

//    return a.exec();
    return 0;
}
