#ifndef ENCODER_H
#define ENCODER_H

#include <QObject>
#include <QSize>

class AVCodec;
class AVCodecContext;
class AVFrame;
class AVDictionary;
class AVStream;
class AVFormatContext;
class ImageConvert;

class Encoder : public QObject
{
    Q_OBJECT
public:
    explicit Encoder(const QSize &s, const QString &n, QObject *parent = 0);
    ~Encoder();

signals:

public slots:
    void onImage(const QImage &img);
    void finish();
protected:
    QSize base_size;
    QString name;
    ImageConvert* converter;
    AVCodec* codec;
    AVCodecContext* context;
    AVFrame* frame;
    AVFormatContext* format;
    AVDictionary *d;
    AVStream *stream;
    int frame_count;
};

#endif // ENCODER_H
