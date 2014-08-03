#ifndef ENCODER_H
#define ENCODER_H

#include <QSize>
#include <QString>

class AVCodec;
class AVCodecContext;
class AVFrame;
class AVDictionary;
class AVStream;
class AVFormatContext;
class ImageConvert;
class QImage;

class Encoder
{
public:
    explicit Encoder(const QSize &s,
                     const QString &n,
                     const QString &config);
    ~Encoder();

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
