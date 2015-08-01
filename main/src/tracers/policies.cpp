/*
 * policies.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tracers/policies.h"
#include "tools/globallog.h"
#include <sstream>

void n_tracers::FileWriter::initialize(const std::string& fileName, bool append)
{
	if (isInitialized())
		n_tools::takeBack(m_stream);
	m_filename = fileName;
	m_stream = new std::ofstream(fileName, append ? std::ofstream::app : std::ofstream::out);
	if (!(m_stream->good() && m_stream->is_open())) {
		n_tools::takeBack(m_stream);
		m_stream = nullptr;
		m_filename.clear();
		throw std::ios_base::failure("FileWriter::initialize Failed to initialize.");
	}
	assert(m_stream->is_open());	//the stream should be open.
}

void n_tracers::FileWriter::startTracer(bool recover)
{
	assert(isInitialized());
	m_disabled = false;
	if (recover) {
		n_tools::takeBack(m_stream);
		m_stream = n_tools::createRawObject<std::ofstream>(m_filename, std::ofstream::app);
		if (!(m_stream->good() && m_stream->is_open())) {
			std::ios_base::failure("FileWriter::startTracer Failed to open file for output.");
			n_tools::takeBack(m_stream);
			m_stream = nullptr;
		}
	}
	assert(m_stream->is_open());	//the stream should be open.
}

void n_tracers::FileWriter::stopTracer()
{
	m_disabled = true;
	m_stream->flush();
}

bool n_tracers::FileWriter::isInitialized() const
{
	return (m_stream != nullptr && !m_filename.empty());
}

n_tracers::FileWriter::~FileWriter()
{
	n_tools::takeBack(m_stream);	//delete should cope with nullptr. It should also close the stream and clear the buffer
}

n_tracers::FileWriter::FileWriter()
	: m_stream(nullptr), m_disabled(false)
{
}

n_tracers::FileWriter::FileWriter(FileWriter&& other)
	: m_stream(std::move(other.m_stream)),
	  m_disabled(std::move(other.m_disabled)),
	  m_filename(std::move(other.m_filename))
{
}



n_tracers::MultiFileWriter::~MultiFileWriter()
{
}

void n_tracers::MultiFileWriter::startNewFile()
{
	assert(isInitialized());
	if (m_disabled)
		return;		//this way, the check is not performed for each argument
	//open a new file
	if(m_stream.is_open())
		m_stream.close();
	std::ostringstream ssr;
	ssr << m_filename << '_' << (m_fileCount++) << m_fileExtend;
	LOG_DEBUG("opening new file: ", ssr.str());
	m_stream.open(ssr.str());
}

void n_tracers::MultiFileWriter::closeFile()
{
	m_stream.close();
}

n_tracers::MultiFileWriter::MultiFileWriter()
	: m_disabled(false), m_fileCount(0)
{
}

void n_tracers::MultiFileWriter::initialize(const std::string& fileName, std::string extend)
{
	m_filename = fileName;
	m_fileExtend = extend;
}

void n_tracers::MultiFileWriter::stopTracer()
{
	m_disabled = true;
}

void n_tracers::MultiFileWriter::startTracer()
{
	m_disabled = false;
}

bool n_tracers::MultiFileWriter::isInitialized() const
{
	return (!m_filename.empty());
}

n_tracers::CoutWriter::CoutWriter()
	: m_disabled(false)
{
}

n_tracers::CoutWriter::CoutWriter(const CoutWriter& other)
	: m_disabled(other.m_disabled)
{
}

n_tracers::CoutWriter::CoutWriter(CoutWriter&& other)
	: m_disabled(std::move(other.m_disabled))
{
}

void n_tracers::CoutWriter::stopTracer()
{
	m_disabled = true;
}

void n_tracers::CoutWriter::startTracer(bool)
{
	m_disabled = false;
}
