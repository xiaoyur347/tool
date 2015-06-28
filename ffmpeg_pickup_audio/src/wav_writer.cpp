#include "wav_writer.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//copy from libsanitizer
#define IMPL_PASTE(a, b) a##b
#define IMPL_COMPILER_ASSERT(pred, line) \
    typedef char IMPL_PASTE(assertion_failed_##_, line)[2*(int)(pred)-1] 
#define COMPILER_CHECK(pred) IMPL_COMPILER_ASSERT(pred, __LINE__) __attribute__((unused))

CWavWriter::CWavWriter()
	:m_file(-1),
	 m_nDataSize(0),
	 m_eType(Type_eFILE)
{
	COMPILER_CHECK(sizeof(WAVE_FORMAT_t)==16);
	COMPILER_CHECK(sizeof(FMT_BLOCK_t)==24);
	COMPILER_CHECK(sizeof(FACT_BLOCK_t)==8);
	COMPILER_CHECK(sizeof(DATA_BLOCK_t)==8);
	COMPILER_CHECK(sizeof(RIFF_HEADER_t)==12);
}

CWavWriter::~CWavWriter()
{
	Close();
}

void CWavWriter::SetType(Type_t type)
{
	m_eType = type;
}

CWavWriter::Type_t CWavWriter::GetType() const
{
	return m_eType;
}

void CWavWriter::SetChannels(unsigned wChannels)
{
	m_FmtParam.wavFormat.SetChannels((uint16_t)wChannels);
}

unsigned CWavWriter::GetChannels() const
{
	return m_FmtParam.wavFormat.GetChannels();
}

void CWavWriter::SetSampleRate(unsigned dwSamplesPerSec)
{
	m_FmtParam.wavFormat.SetSampleRate((uint32_t)dwSamplesPerSec);
}

unsigned CWavWriter::GetSampleRate() const
{
	return m_FmtParam.wavFormat.GetSampleRate();
}

void CWavWriter::SetName(const char *szName)
{
	m_strPath = szName;
}

bool CWavWriter::Create()
{
	if (m_eType == Type_eFILE)
	{
		return true;
	}
	
	if (m_strPath.empty())
	{
		return false;
	}
	
	unlink(m_strPath.c_str());
	if (mkfifo (m_strPath.c_str(), S_IFIFO | 0777) == -1)
	{
		return false;
	}

	return true;
}

bool CWavWriter::Open()
{
	if (m_file >= 0)
	{
		return true;
	}

	m_nDataSize = 0;
	m_RiffParam.Clear();
	m_DataParam.Clear();

	if (m_eType == Type_eFILE)
	{
		m_file = open(m_strPath.c_str(),
						(O_CREAT | O_WRONLY | O_TRUNC),
						0666);
	}
	else
	{
		m_file = open(m_strPath.c_str(), (O_WRONLY | O_NONBLOCK), 0666);
	}

	if (m_file < 0)
	{
		return false;
	}
	
	write(m_file, &m_RiffParam, sizeof(RIFF_HEADER_t));
	write(m_file, &m_FmtParam, sizeof(FMT_BLOCK_t));
	write(m_file, &m_DataParam, sizeof(DATA_BLOCK_t));
	return true;
}

unsigned int CWavWriter::Write(const void *buf, unsigned int nbyte)
{
	if (m_file < 0)
	{
		return 0;
	}
	
	m_nDataSize += nbyte;
	
	m_DataParam.SetDataLength(m_nDataSize);
	m_RiffParam.SetFileLength(m_nDataSize);
	
	if (buf != NULL)
	{
		write(m_file, buf, nbyte);
	}
	
	return nbyte;
}

bool CWavWriter::Close()
{
	if (m_file < 0)
	{
		return false;
	}

	if (m_eType == Type_eFILE)
	{
		lseek(m_file, 0, SEEK_SET);
		write(m_file, &m_RiffParam, sizeof(RIFF_HEADER_t));
		lseek(m_file, sizeof(RIFF_HEADER_t) + sizeof(FMT_BLOCK_t), SEEK_SET);
		write(m_file, &m_DataParam, sizeof(DATA_BLOCK_t));
	}
	int nFile = m_file;
	m_file = -1;
	close(nFile);
	
	return true;
}

unsigned int CWavWriter::GetSecond() const
{
	unsigned int time = m_nDataSize / m_FmtParam.wavFormat.GetBytesPerSec();
	return time;
}


