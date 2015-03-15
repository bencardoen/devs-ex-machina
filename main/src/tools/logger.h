/*
 * logger.h
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TOOLS_LOGGER_H_
#define SRC_TOOLS_LOGGER_H_

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
	class LockedQueue: public std::queue<T, C>{
		private:
			std::mutex m_mutex;
		public:
			using std::queue<T, C>::queue;
			bool empty(){
				std::lock_guard<std::mutex> guard(this->m_mutex);
				return std::queue<T, C>::empty();
			}

			void push(T item){
				std::lock_guard<std::mutex> guard(this->m_mutex);
				std::queue<T, C>::push(item);
			}

	};

	//http://stackoverflow.com/a/21127776
	struct async_buf
	    : std::streambuf
	{
	    std::ofstream                 out;
	    std::mutex                    mutex;
	    std::condition_variable       condition;
	    LockedQueue<std::vector<char>> queue;
	    std::vector<char>             buffer;
	    std::thread                   thread;
	    bool                          done;

	    void worker() {
	        bool local_done(false);
	        std::vector<char> buf;
	        while (true) {
	            {
	                std::unique_lock<std::mutex> guard(this->mutex);
	                if(!isQueueEmpty() || !local_done) break;
	                this->condition.wait(guard,
	                                     [this](){ return !this->queue.empty()
	                                                   || this->done; });
	                while (isQueueEmpty() && !this->done) {
	                    this->condition.wait(guard);
	                }
	                if (!isQueueEmpty()) {
	                    buf.swap(queue.front());
	                    queue.pop();
	                }
	                local_done = this->done;
	            }
	            if (!buf.empty()) {
	                out.write(buf.data(), std::streamsize(buf.size()));
	                out.flush();
	                buf.clear();
	            }
	        }
	    }

	    bool isQueueEmpty(){
	    	return this->queue.empty();
	    }

	public:
	    async_buf(std::string const& name)
	        : out(name)
	        , buffer(512)
	        , thread(std::bind(&async_buf::worker, this)) {
	        this->setp(this->buffer.data(),
	                   this->buffer.data() + this->buffer.size() - 1);
	    }
	    ~async_buf() {
	        {
	                std::unique_lock<std::mutex> guard(this->mutex);
	                this->done = true;
	        }
	        this->condition.notify_one();
	        this->thread.join();
	    }
	    int overflow(int c) {
	        if (c != std::char_traits<char>::eof()) {
	            *this->pptr() = std::char_traits<char>::to_char_type(c);
	            this->pbump(1);
	        }
	        return this->sync()
	            ? std::char_traits<char>::not_eof(c): std::char_traits<char>::eof();
	    }
	    int sync() {
            std::lock_guard<std::mutex> guard(this->mutex);
	        if (this->pbase() != this->pptr()) {
	            this->buffer.resize(std::size_t(this->pptr() - this->pbase()));
	            {
	                this->queue.push(std::move(this->buffer));
	            }
	            this->condition.notify_one();
	            this->buffer = std::vector<char>(128);
	            this->setp(this->buffer.data(),
	                       this->buffer.data() + this->buffer.size() - 1);
	            return 1;
	        }
	        return 0;
	    }

	};

	class Logger {
		public:
	    		enum LoggingLevel{
	    			E_ERROR = 1,
					E_WARNING = 2,
					E_DEBUG = 4,
	    			E_INFO = 8
	    		};
	    	Logger(const char* filename, int level = E_ERROR | E_WARNING | E_DEBUG | E_INFO);
	    	~Logger();

	    	void startEntry(LoggingLevel level);

	    	template<typename T>
	    	Logger& operator<<(const T& data){
//	    		if(m_doPrint)
	    		std::lock_guard<std::mutex> m(this->m_mutex);
	    			m_out << data;
	    		return *this;
	    	}

		private:
	    	async_buf* m_buf;
	    	std::ostream m_out;
	    	int m_levelFilter;
	    	bool m_doPrint;
	    	std::mutex m_mutex;

	};

	std::ostream& operator<<(std::ostream& out, Logger::LoggingLevel level);


} /* namespace n_tools */

//you can define the log level here. if you want to
#define LOG_LEVEL 8

#endif /* SRC_TOOLS_LOGGER_H_ */
