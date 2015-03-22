/*
 * asynchwriter.h
 *
 *  Created on: Mar 15, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TOOLS_ASYNCHWRITER_H_
#define SRC_TOOLS_ASYNCHWRITER_H_

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

namespace n_tools {
template<typename T, typename C = std::deque<T> >
class LockedQueue: public std::queue<T, C>
{
private:
	std::mutex m_mutex;
public:
	using std::queue<T, C>::queue;
	bool empty()
	{
		std::lock_guard<std::mutex> guard(this->m_mutex);
		return std::queue<T, C>::empty();
	}

	void push(T item)
	{
		std::lock_guard<std::mutex> guard(this->m_mutex);
		std::queue<T, C>::push(item);
	}

};

//http://stackoverflow.com/a/21127776
struct ASynchWriter: std::streambuf
{
	std::ofstream m_out;
	std::mutex m_mutex;
	std::condition_variable m_condition;
	LockedQueue<std::vector<char>> m_queue;
	std::vector<char> m_buffer;
	std::thread m_thread;
	std::atomic<bool> m_done;

	void worker()
	{
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

public:
	ASynchWriter(std::string const& name)
		: m_out(name), m_buffer(512), m_thread(std::bind(&ASynchWriter::worker, this)), m_done(false)
	{
		std::lock_guard<std::mutex> lock(this->m_mutex);
		this->setp(this->m_buffer.data(), this->m_buffer.data() + this->m_buffer.size() - 1);
	}
	~ASynchWriter()
	{
		{
			std::unique_lock<std::mutex> guard(this->m_mutex);
			this->m_done = true;
		}
		this->m_condition.notify_one();
		this->m_thread.join();
	}
	int overflow(int c)
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
	int sync()
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

};
} /* namespace n_tools */

#endif /* SRC_TOOLS_ASYNCHWRITER_H_ */
