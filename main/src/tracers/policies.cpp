/*
 * policies.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "policies.h"

void n_tracers::FileWriter::initialize(const std::string& fileName, bool append)
{
	if (isInitialized())
		delete m_stream;
	m_filename = fileName;
	m_stream = new std::ofstream(fileName, append ? std::ofstream::app : std::ofstream::out);
	if (!(m_stream->good() && m_stream->is_open())) {
		delete m_stream;
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
		delete m_stream;
		m_stream = new std::ofstream(m_filename, std::ofstream::app);
		if (!(m_stream->good() && m_stream->is_open())) {
			std::ios_base::failure("FileWriter::startTracer Failed to open file for output.");
			delete m_stream;
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
	delete m_stream;	//delete should cope with nullptr. It should also close the stream and clear the buffer
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

n_tracers::CoutWriter::~CoutWriter()
{
}
