/*
 * asynchwriter.cpp
 *
 *  Created on: Apr 2, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "asynchwriter.h"
#include <cassert>

namespace n_tools{

void ASynchWriter::worker()
{
	assert(m_out.is_open() && "AsynchWriter::worker Failed to open the file for output.");
	std::atomic<bool> local_done(false);
	std::vector<char> buf;
	while (true) {
		{
			std::unique_lock<std::mutex> guard(this->m_mutex);
			if (m_queue.empty() && local_done) {
				guard.unlock();
				return;
			}

			this->m_condition.wait(guard, [this]()->bool {return
				(!m_queue.empty()
					|| m_done);});
			while (m_queue.empty() && !this->m_done) {
				this->m_condition.wait(guard);
			}

			if (!m_queue.empty()) {
				buf.swap(m_queue.front());
				m_queue.pop();
			}
			local_done.store(this->m_done);
		}
		if (!buf.empty()) {
			m_out.write(buf.data(), std::streamsize(buf.size()));
			m_out.flush();
			buf.clear();
		}
	}
}

ASynchWriter::ASynchWriter(const std::string& name)
	: m_out(name), m_buffer(512), m_done(false), m_thread(std::bind(&ASynchWriter::worker, this))
{
	assert(m_out.is_open() && "AsynchWriter::AsynchWriter Failed to open the file for output.");

	std::lock_guard<std::mutex> lock(this->m_mutex);
	this->setp(this->m_buffer.data(), this->m_buffer.data() + this->m_buffer.size() - 1);
}

ASynchWriter::~ASynchWriter()
{
	{
		std::unique_lock<std::mutex> guard(this->m_mutex);
		this->m_done = true;
	}
	this->m_condition.notify_one();
	this->m_thread.join();
}

int ASynchWriter::overflow(int c)
{
	{
		std::lock_guard<std::mutex> guard(this->m_mutex);
		if (c != std::char_traits<char>::eof()) {
			*this->pptr() = std::char_traits<char>::to_char_type(c);
			this->pbump(1);
		}
	}
	return this->sync() ? std::char_traits<char>::not_eof(c) : std::char_traits<char>::eof();
}

int ASynchWriter::sync()
{
	std::lock_guard<std::mutex> guard(this->m_mutex);
	if (this->pbase() != this->pptr()) {
		this->m_buffer.resize(std::size_t(this->pptr() - this->pbase()));
		{
			this->m_queue.push(std::move(this->m_buffer));
		}
		this->m_condition.notify_one();
		this->m_buffer = std::vector<char>(128);
		this->setp(this->m_buffer.data(), this->m_buffer.data() + this->m_buffer.size() - 1);
		return 1;
	}
	return 0;
}

} /* namespace n_tools */

