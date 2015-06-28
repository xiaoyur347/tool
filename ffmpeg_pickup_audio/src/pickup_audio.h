#ifndef PICKUP_AUDIO_H
#define PICKUP_AUDIO_H

#define __STDC_CONSTANT_MACROS
#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#endif

#include <pthread.h>
#include <string>

class CPickupAudio
{
public:
	enum Error_t
	{
		Error_eNone,
		Error_eInputFileOpen,
		Error_eInputFileNoAudio,
		Error_eInputFileAudioFormat,
		Error_eInputFileAudioNotSupport,
		Error_eOutputFileOpen,
		Error_eOutputFileClose
	};
	CPickupAudio();
	~CPickupAudio();
	
	void Start(const char *szInput, const char *szOutput);
	void Stop();
	bool IsRunning() const
	{
		return m_bRunning;
	}
	Error_t GetError() const
	{
		return m_nError;
	}

private:
	static void *ReadThread(void *);
	void Loop();
	
	bool Open();
	bool OpenInput();
	bool OpenOutput();
	void Read();
	void Close();
	
	int Decode(const AVPacket *pkt, uint8_t **decBuffer);
	
	std::string m_strInput;
	std::string m_strOutput;
	
	Error_t m_nError;
	bool m_bRunning;
	bool m_bEof;
	pthread_t m_pid;
	AVFormatContext *m_pInFormatContext;
	AVCodecContext *m_pAudioCodec;
	struct SwrContext *m_pSwrContext;
	uint8_t *m_pSwDecBuffer;
	
	int m_nAudioIndex;
	int m_nAudioChannel;
	int m_nAudioSampleRate;
	
	class CWavWriter *m_pWriter;
};

#endif //PICKUP_AUDIO_H
