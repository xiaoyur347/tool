#include "pickup_audio.h"
#include "wav_writer.h"
#include <stdio.h>

CPickupAudio::CPickupAudio()
	:m_nError(Error_eNone),
	 m_bRunning(false),
	 m_bEof(false),
	 m_pid(0),
	 m_pInFormatContext(NULL),
	 m_pAudioCodec(NULL),
	 m_pSwrContext(NULL),
	 m_pSwDecBuffer(NULL),
	 m_nAudioIndex(-1),
	 m_nAudioChannel(0),
	 m_nAudioSampleRate(0),
	 m_pWriter(NULL)
{
}

CPickupAudio::~CPickupAudio()
{
}

void CPickupAudio::Start(const char *szInput, const char *szOutput)
{
	if (m_bRunning)
	{
		return;
	}
	
	m_bRunning = true;
	m_bEof = false;
	if (szInput)
	{
		m_strInput = szInput;
	}
	if (szOutput)
	{
		m_strOutput = szOutput;
	}
	
	pthread_create(&m_pid, NULL, ReadThread, this);
}

void CPickupAudio::Stop()
{
	if (!m_bRunning)
	{
		if (m_pid != 0)
		{
			pthread_join(m_pid, NULL);
			m_pid = 0;
		}
		return;
	}
	
	m_bRunning = false;
	pthread_join(m_pid, NULL);
	m_pid = 0;
}

void *CPickupAudio::ReadThread(void *arg)
{
	CPickupAudio *pThis = (CPickupAudio *)arg;
	pThis->Loop();
	return NULL;
}

void CPickupAudio::Loop()
{
	Open();
	
	while (m_bRunning && m_nError == Error_eNone && !m_bEof)
	{
		Read();
	}
	
	Close();
	
	m_bRunning = false;
}

bool CPickupAudio::OpenInput()
{
	m_pInFormatContext = avformat_alloc_context();
	m_pInFormatContext->probesize = 128*1024;
	
	AVDictionary *d = NULL;
	int ret = avformat_open_input(&m_pInFormatContext, m_strInput.c_str(), NULL, &d);
	av_dict_free(&d);
	if (ret != 0)
	{
		m_nError = Error_eInputFileOpen;
		return false;
	}
	
	avformat_find_stream_info(m_pInFormatContext, NULL);
	av_dump_format(m_pInFormatContext, 0, m_strInput.c_str(), 0);
	
	for(unsigned i=0; i<m_pInFormatContext->nb_streams; i++)
	{
		AVStream *stream = m_pInFormatContext->streams[i];
		if (stream->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			if (!stream->codec->sample_rate)
			{
				m_nError = Error_eInputFileAudioFormat;
				return false;
			}
			if (m_nAudioIndex==-1)
			{
				m_nAudioIndex = i;
				m_nAudioChannel = stream->codec->channels;
				m_nAudioSampleRate = stream->codec->sample_rate;
				m_pAudioCodec = stream->codec;
				AVCodec *inAcodec = avcodec_find_decoder(m_pAudioCodec->codec_id);
				if (inAcodec == NULL)
				{
					m_nError = Error_eInputFileAudioNotSupport;
					return false;
				}
				if (m_pAudioCodec->sample_fmt == AV_SAMPLE_FMT_S16P)
				{
					m_pAudioCodec->request_sample_fmt = AV_SAMPLE_FMT_S16;
				}
				avcodec_open2(m_pAudioCodec, inAcodec, NULL);
				break;
			}
		}
	}
	
	if (m_nAudioIndex < 0)
	{
		m_nError = Error_eInputFileNoAudio;
		return false;
	}

	return true;
}

bool CPickupAudio::OpenOutput()
{
	m_pWriter = new CWavWriter;
	m_pWriter->SetChannels(m_nAudioChannel);
	m_pWriter->SetSampleRate(m_nAudioSampleRate);
	m_pWriter->SetName(m_strOutput.c_str());
	if (!m_pWriter->Open())
	{
		m_nError = Error_eOutputFileOpen;
		return false;
	}
	
	return true;
}

bool CPickupAudio::Open()
{
	if (!OpenInput())
	{
		return false;
	}
	
	if (!OpenOutput())
	{
		return false;
	}
	
	return true;
}

int CPickupAudio::Decode(const AVPacket *pkt, uint8_t **decBuffer)
{
	AVFrame decFrame;
	memset(&decFrame, 0, sizeof(AVFrame));
	int got_frame = 0;
	avcodec_decode_audio4(m_pAudioCodec, &decFrame, &got_frame, pkt);
	if(!got_frame)
	{
		printf("avcodec_decode_audio4 failed\n");
		return -1;
	}
	
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int channels = av_frame_get_channels(&decFrame);

	int data_size = av_samples_get_buffer_size(NULL, channels,
								   decFrame.nb_samples,
								   (enum AVSampleFormat)decFrame.format, 1);
	int64_t dec_channel_layout =
					(decFrame.channel_layout && channels == av_get_channel_layout_nb_channels(decFrame.channel_layout)) ?
					decFrame.channel_layout : av_get_default_channel_layout(channels);
	if(decFrame.format != out_sample_fmt)
	{
		if(!m_pSwrContext)
		{
			printf("swr init\n");
			m_pSwrContext = swr_alloc_set_opts(NULL,
								dec_channel_layout, out_sample_fmt, decFrame.sample_rate,
								dec_channel_layout, (enum AVSampleFormat)decFrame.format, decFrame.sample_rate,
								0, NULL);
			if (!m_pSwrContext || swr_init(m_pSwrContext) < 0) {
				printf("error: swr_init failed\n");
				return -1;
			}
		}
		if (m_pSwrContext) {
			const uint8_t **in = (const uint8_t **)decFrame.extended_data;
			uint8_t **out = &m_pSwDecBuffer;
			unsigned int swDecBufferSize = 0;
			int out_count = 0;
			out_count = (int64_t)decFrame.nb_samples;
			int out_size  = av_samples_get_buffer_size(NULL, 
				channels, out_count, 
				out_sample_fmt, 0);
			av_fast_malloc(&m_pSwDecBuffer, &swDecBufferSize, out_size);
			if (!m_pSwDecBuffer)
			{
				printf("av_fast_malloc failed\n");
				return -1;
			}
			int len2 = swr_convert(m_pSwrContext, out, out_count, in, decFrame.nb_samples);
			if (len2 < 0) {
				printf("swr_convert() failed\n");
				return -1;
			}
			int resampled_data_size = len2 * channels
				* av_get_bytes_per_sample(out_sample_fmt);
			data_size = resampled_data_size;
			*decBuffer = m_pSwDecBuffer;
		}
	}
	else
	{
		*decBuffer = decFrame.data[0];
	}
	return data_size;
}

void CPickupAudio::Read()
{
	AVPacket pkt;
	av_init_packet(&pkt);

	int ret = av_read_frame(m_pInFormatContext, &pkt);
	if (ret == AVERROR_EOF)
	{
		m_bRunning = false;
	}
	else if (ret == AVERROR(EAGAIN))
	{
		printf("need another av_read_frame\n");
	}
	else if (ret < 0)
	{
		printf("av_read_frame return %d\n", ret);
	}
	else if (ret >= 0)
	{
		if (pkt.stream_index != m_nAudioIndex)
		{
			av_free_packet(&pkt);
			return;
		}
		uint8_t *decBuffer = NULL;
		int nSize = Decode(&pkt, &decBuffer);
		if (nSize > 0)
		{
			m_pWriter->Write(decBuffer, nSize);
		}

		av_free_packet(&pkt);
	}
}

void CPickupAudio::Close()
{
	if (m_pAudioCodec)
	{
		avcodec_close(m_pAudioCodec);
		m_pAudioCodec = NULL;
	}
	if (m_pSwDecBuffer)
	{
		av_freep(&m_pSwDecBuffer);
		m_pSwDecBuffer = NULL;
	}
	if (m_pSwrContext)
	{
		swr_free(&m_pSwrContext);
		m_pSwrContext = NULL;
	}
	if (m_pInFormatContext)
	{
		avformat_close_input(&m_pInFormatContext);
		m_pInFormatContext = NULL;
	}
	if (m_pWriter)
	{
		m_pWriter->Close();
		delete m_pWriter;
		m_pWriter = NULL;
	}
}

