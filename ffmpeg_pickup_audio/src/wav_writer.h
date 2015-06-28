#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <vector>
#include <string>
#include <stdint.h>

struct WAVE_FORMAT_t
{
	uint16_t wFormatTag;
	uint16_t wChannels;
	uint32_t dwSamplesPerSec;
	uint32_t dwAvgBytesPerSec;
	uint16_t wBlockAlign;
	uint16_t wBitsPerSample;
	
	enum
	{
		BYTES_PER_CHANNEL = 2,
	};
	
	WAVE_FORMAT_t()
	{
		wFormatTag = 1;//pcm
		wBitsPerSample = BYTES_PER_CHANNEL * 8;
		
		wChannels = 2;
		wBlockAlign = wChannels * BYTES_PER_CHANNEL;
		
		dwSamplesPerSec = 48000;
		dwAvgBytesPerSec = dwSamplesPerSec * wBlockAlign;
	}
	
	uint32_t GetBytesPerSec() const
	{
		return dwAvgBytesPerSec;
	}
	
	void SetSampleRate(uint32_t samplerate)
	{
		dwSamplesPerSec = samplerate;
		
		dwAvgBytesPerSec = dwSamplesPerSec * wBlockAlign;
	}
	
	uint32_t GetSampleRate() const
	{
		return dwSamplesPerSec;
	}
	
	void SetChannels(uint16_t channels)
	{
		wChannels = channels;
		wBlockAlign = wChannels * BYTES_PER_CHANNEL;
		
		dwAvgBytesPerSec = dwSamplesPerSec * wBlockAlign;
	}
	
	uint16_t GetChannels() const
	{
		return wChannels;
	}
};

struct FMT_BLOCK_t//format
{
	char szFmtID[4]; // 'f','m','t',' '
	uint32_t dwFmtSize;//////////////16 or 18(extend)
	WAVE_FORMAT_t wavFormat;
	
	FMT_BLOCK_t()
	{
		szFmtID[0] = 'f';
		szFmtID[1] = 'm';
		szFmtID[2] = 't';
		szFmtID[3] = ' ';
		dwFmtSize = 16;
	}
};

struct FACT_BLOCK_t//optional
{
	char szFactID[4]; // 'f','a','c','t'
	uint32_t dwFactSize;
	
	FACT_BLOCK_t()
	{
		szFactID[0] = 'f';
		szFactID[1] = 'a';
		szFactID[2] = 'c';
		szFactID[3] = 't';
		dwFactSize = 0;
	}
};

struct DATA_BLOCK_t//data head
{
	char szDataID[4]; // 'd','a','t','a'
	uint32_t dwDataSize;
	
	DATA_BLOCK_t()
	{
		szDataID[0] = 'd';
		szDataID[1] = 'a';
		szDataID[2] = 't';
		szDataID[3] = 'a';
		Clear();
	}
	
	void Clear()
	{
		dwDataSize = 0;
	}
	
	void SetDataLength(uint32_t length)
	{
		dwDataSize = length;
	}
	uint32_t GetDataLength() const
	{
		return dwDataSize;
	}
};

struct RIFF_HEADER_t//file header
{
	char szRiffID[4]; // 'R','I','F','F'
	uint32_t dwRiffSize;
	char szRiffFormat[4]; // 'W','A','V','E'
	
	RIFF_HEADER_t()
	{
		Clear();
	}
	
	void Clear()
	{
		szRiffID[0] = 'R';
		szRiffID[1] = 'I';
		szRiffID[2] = 'F';
		szRiffID[3] = 'F';
		szRiffFormat[0] = 'W';
		szRiffFormat[1] = 'A';
		szRiffFormat[2] = 'V';
		szRiffFormat[3] = 'E';
		
		SetFileLength(0);
	}
	
	void SetFileLength(uint32_t length)
	{
		dwRiffSize = length
					 + sizeof(FMT_BLOCK_t)
					 + sizeof(DATA_BLOCK_t)
					 + sizeof(RIFF_HEADER_t) - 8;
	}
	uint32_t GetFileLength() const
	{
		return dwRiffSize;
	}
};


class CWavWriter
{
public:
	enum Type_t
	{
		Type_eFILE,
		Type_eFIFO,
	};
	CWavWriter();
	~CWavWriter();
	
	//The below functions must be called before Create/Open
	void SetType(Type_t type);
	Type_t GetType() const;
	
	void SetChannels(unsigned nChannels);
	unsigned GetChannels() const;
	void SetSampleRate(unsigned dwSamplesPerSec);
	unsigned GetSampleRate() const;

	void SetName(const char *szName);
	//The above functions must be called before Create/Open
	
	bool Create();
	bool Open(); //Open for write
	
	unsigned int Write(const void *buf, unsigned int nbyte);
	bool Close();
	
	unsigned int GetSecond() const;
private:
	RIFF_HEADER_t m_RiffParam;
	FMT_BLOCK_t m_FmtParam;
	FACT_BLOCK_t m_FactParam;
	DATA_BLOCK_t m_DataParam;
	std::string m_strPath;
	int m_file;
	unsigned int m_nDataSize;
	Type_t m_eType;
};

#endif // WAV_WRITER_H


