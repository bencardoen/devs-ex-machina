/*
 * asynchwriter.h
 *
 *  Created on: Mar 15, 2015
 *      Author: lttlemoi
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

namespace n_tools{
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
	struct ASynchWriter
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
	    ASynchWriter(std::string const& name)
	        : out(name)
	        , buffer(512)
	        , thread(std::bind(&ASynchWriter::worker, this)) {
	        this->setp(this->buffer.data(),
	                   this->buffer.data() + this->buffer.size() - 1);
	    }
	    ~ASynchWriter() {
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
}	/* namespace n_tools */


#endif /* SRC_TOOLS_ASYNCHWRITER_H_ */
