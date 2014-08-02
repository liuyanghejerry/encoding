#include "encoder.h"

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

extern "C" {
#ifdef __cplusplus

#define __STDC_CONSTANT_MACROS

#ifdef _STDINT_H

#undef _STDINT_H

#endif

# include <stdint.h>

#endif

#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
}


#include <QImage>
#include <QPainter>
#include <QDebug>

static const int fps = 30;

class ImageConvert
{
public:
    ImageConvert(const QSize &size)
    {
        ctx = sws_getContext(size.width(),
                              size.height(),
                              AV_PIX_FMT_RGB32,
                              size.width(),
                              size.height(),
                              AV_PIX_FMT_YUV420P,
                              0, 0, 0, 0);
    }

    ~ImageConvert()
    {
        sws_freeContext(ctx);
    }

    void convert(const QImage &image, AVFrame* frame)
    {
        uint8_t * inData[1] = { (uint8_t*)image.constBits() }; // RGB have one plane
        int inLinesize[1] = { 4*image.width() }; // RGB stride
        sws_scale(ctx, inData, inLinesize, 0, image.height(),
                  frame->data, frame->linesize);
    }
private:
    SwsContext * ctx;
};

Encoder::Encoder(const QSize &s, const QString &n, QObject *parent) :
    QObject(parent),
    base_size(s),
    name(n),
    converter(new ImageConvert(s))
{
    int ret = 0;
    avcodec_register_all();
    av_register_all();
    codec = NULL;
    context = NULL;
    frame = NULL;
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    format = avformat_alloc_context();
    format->oformat = av_guess_format("mkv", n.toStdString().c_str(), NULL);
    if (!format->oformat) {
        qDebug()<<"cannot output mkv, use mpeg instead";
        format->oformat = av_guess_format("mpeg", NULL, NULL);
    }
    format->video_codec_id = format->oformat->video_codec;
    if(!codec){
        qDebug()<<"cannot find encoder";
        return;
    }
    qDebug()<<"encoder found";

    context = avcodec_alloc_context3(codec);
    if(!context){
        qDebug()<<"cannot alloc context";
        return;
    }
    qDebug()<<"context allocted";

    context->bit_rate = 400000;
    // resolution must be a multiple of two
    context->width = base_size.width();
    context->height = base_size.height();
    // frames per second
    context->time_base= (AVRational){1,fps};
    context->gop_size = 12; // emit one intra frame every twelve frames at most
//    context->max_b_frames = 1; // FIXME: will flash if too large
    context->pix_fmt = AV_PIX_FMT_YUV420P;
    qDebug()<<"context init";

    d = NULL;

    ret = avcodec_open2(context, codec, &d);
    if ( ret < 0) {
        fflush(stderr);
        qDebug()<<"cannot open codec"<<ret;
        return;
    }
    qDebug()<<"codec open";

    frame = av_frame_alloc();
    if (!frame) {
        qDebug()<<"Could not allocate video frame";
        return;
    }
    qDebug()<<"frame allocated";

    frame->format = context->pix_fmt;
    frame->width  = context->width;
    frame->height = context->height;


    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize,
                         context->width, context->height,
                         context->pix_fmt, 32);
    qDebug()<<"image allocated";

    frame_count = 0;

    stream = avformat_new_stream(format, codec);
    if(!stream) {
       printf("Could not allocate stream\n");
    }
    stream->codec = context;

    if(format->oformat->flags & AVFMT_GLOBALHEADER) {
       context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    qDebug()<<name.toStdString().c_str();
    avio_open(&format->pb,
              name.toStdString().c_str(),
              AVIO_FLAG_WRITE);
    avformat_write_header(format, &d);
}

Encoder::~Encoder()
{
    avcodec_close(context);
    av_free(context);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    avformat_free_context(format);
    avio_close(format->pb);
    delete converter;
}

void Encoder::onImage(const QImage &img)
{
    if(img.isNull()) {
        qDebug()<<"Error input image";
        return;
    }
    int got_output = 0;
    int ret = 0;

    for(int i=0;i<fps;++i) {
        /* encode x second of video */
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        fflush(stdout);

        converter->convert(img, frame);

        int pts = frame_count;

        frame->pts = pts;

        frame_count++;
        /* encode the image */
        ret = avcodec_encode_video2(context, &pkt, frame, &got_output);
        if (ret < 0) {
            qDebug()<<"Error encoding frame";
            return;
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", frame_count, pkt.size);

            context->coded_frame->pts = pts;  // Set the time stamp

            if (context->coded_frame->pts != (0x8000000000000000LL))
            {
                pts = av_rescale_q(context->coded_frame->pts,
                                           context->time_base,
                                           format->streams[0]->time_base);
            }
            pkt.pts = pts;

            if(context->coded_frame->key_frame)
            {
               pkt.flags |= AV_PKT_FLAG_KEY;
            }

            av_interleaved_write_frame(format, &pkt);
            av_free_packet(&pkt);
        }
    }
}

void Encoder::finish()
{
    int ret = 0;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    /* get the delayed frames */
    for (int i = 0, got_output = 1; got_output; i++) {
        fflush(stdout);
        ret = avcodec_encode_video2(context, &pkt, NULL, &got_output);
        if (ret < 0) {
            qDebug()<<"Error encoding frame";
            return;
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", frame_count, pkt.size);
            frame_count++;
            av_interleaved_write_frame(format, &pkt);
        }
        av_free_packet(&pkt);
    }
    av_write_trailer(format);
}

