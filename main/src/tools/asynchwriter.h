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
#include <atomic>
#include <deque>
#include <iostream>
#include <set>

namespace n_tools {
/**
 * @brief Thread safe Container class.
 * @tparam T The type of objects that is stored in this container
 * @tparam C [Optional, default = std::deque<T>] The actual container used for storing the values.
 */
template<typename T, typename C = std::deque<T> >
class LockedQueue: public std::queue<T, C>
{
private:
	std::mutex m_mutex;
public:
	using std::queue<T, C>::queue;
	/**
	 * @brief Tests whether or not the queue is empty
	 * @retval true The container does not contain any values.
	 * @retval false The container contains values.
	 */
	bool empty()
	{
		std::lock_guard<std::mutex> guard(this->m_mutex);
		return std::queue<T, C>::empty();
	}

	/**
	 * @brief Pushes a new value at the end of the container.
	 * @param item The new value that is placed in the container.
	 */
	void push(T item)
	{
		std::lock_guard<std::mutex> guard(this->m_mutex);
		std::queue<T, C>::push(item);
	}

};

/**
 * @brief Asynchronous writer. Can write stuff to a file in an asynchronous way.
 * You can turn any output stream (including std::cout) into an asynchronous file output stream
 * by switching its buffer with this class.
 *
 * @see http://stackoverflow.com/a/21127776 Thanks a lot to Dietmar Kühl for providing the basic idea behind this class
 * We messed around with the code quite a bit and fixed some important data races, but he still deserves the credit.
 */
class ASynchWriter: public std::streambuf
{
private:
	std::ofstream m_out;
	std::mutex m_mutex;
	std::condition_variable m_condition;
	LockedQueue<std::vector<char>> m_queue;
	std::vector<char> m_buffer;
	std::atomic<bool> m_done;
	std::thread m_thread;

	void worker();

public:
	/**
	 * @brief Creates a new Asynchronous writer and opens a file to dump the output.
	 * @param name The name of the file that will be opened.
	 * @precondition The system must be able to open the file for output.
	 */
	ASynchWriter(const std::string& name, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc);
	/**
	 * @Brief Destructor, will block the current thread until the asynchronous writer thread has finished.
	 */
	~ASynchWriter();

	/**
	 * std::basic_streambuf::overflow
	 */
	int overflow(int c);
	/**
	 * @see std::basic_streambuf::sync
	 */
	int sync();

};
} /* namespace n_tools */

#endif /* SRC_TOOLS_ASYNCHWRITER_H_ */
