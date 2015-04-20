/*
 * policies.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_POLICIES_H_
#define SRC_TRACERS_POLICIES_H_

#include <fstream>
#include <iostream>
#include <assert.h>

namespace n_tracers {

/**
 * @brief Tracer output policy. Output will be printed to a file.
 */
class FileWriter
{
public:
	/**
	 * @brief Opens the file for output
	 *
	 * @param fileName The name of the file. Output will be written to this file
	 * @param append [optional, default: false] If true, output will be appended to the file, rather than overwriting it.
	 * @postcondition The file is opened and ready for output.
	 * @throws std::ios_base::failure If the file could not be opened for output.
	 */
	void initialize(const std::string& fileName, bool append = false);

	/**
	 * @brief Stops the tracer from generating any further output.
	 * @warning	We assume that this function is not called while a simulation is in progress.
	 * 		If there is, we can make no guarantee when the output generation actually stops.
	 */
	void stopTracer();

	/**
	 * @brief Restarts the tracer.
	 * @param recover Whether or not the new output should be appended to the old output or not. If not, the previous file will be wiped.
	 * @precondition A file has previously been opened and could be written to.
	 * @throws std::ios_base::failure If the previous file could not be opened for output.
	 */
	void startTracer(bool recover);

	/**
	 * @brief Checks whether the policy has been properly initialized. That is, whether a file has been opened.
	 */
	bool isInitialized() const;

protected:
	~FileWriter();
	FileWriter();
	FileWriter(const FileWriter& other) = delete;
	FileWriter(FileWriter&& other);

	/**
	 * @brief Prints the data to a file.
	 *
	 * The data can have any type, as long as the proper output operator has been defined.
	 * This function is called whenever tracing output should be written to the output stream.
	 *
	 * @param data... These objects will be printed to the file.
	 * @precondition The output operator << is specified for printing each and every data object to an std::ofstream.
	 * @precondition A file has been opened and can be written to.
	 */
	template<typename ... Args>
	void print(const Args&... args)
	{
		assert(isInitialized());
		if (m_disabled)
			return;		//this way, the check is not performed for each argument
		printImpl(args...);
	}

private:
	std::ofstream* m_stream;
	bool m_disabled;
	std::string m_filename;

	template<typename T, typename ... Args>
	void printImpl(const T& data, const Args&... args)
	{
		assert(isInitialized());
		*m_stream << data;	//print the data
		printImpl(args...);	//print more data
	}

	/**
	 * @brief print base case. Finalizes printing the data.
	 */
	template<typename ...>
	void printImpl()
	{
		//nothing to do here
	}
};

/**
 * @brief Tracer output policy. Output will be printed to standard out.
 */
class CoutWriter
{
public:
	/**
	 * @brief Stops the tracer from generating any further output.
	 * @warning	We assume that this function is not called while a simulation is in progress.
	 * 		If there is, we can make no guarantee when the output generation actually stops.
	 */
	void stopTracer();

	/**
	 * @brief Restarts the tracer.
	 * @param recover <s>Whether or not the new output should be appended to the old output or not. If not, the previous file will be wiped.</s>
	 * @note Obviously, output can only be appended to the console. The tracer will therefore always start in recovery mode.
	 */
	void startTracer(bool recover);

protected:
	CoutWriter();
	CoutWriter(const CoutWriter& other);
	CoutWriter(CoutWriter&& other);
	~CoutWriter();

	/**
	 * @brief Prints the data to a file.
	 *
	 * The data can have any type, as long as the proper output operator has been defined.
	 * This function is called whenever tracing output should be written to the output stream.
	 *
	 * @param data... These objects will be printed to the file.
	 * @precondition The output operator << is specified for printing each and every data object to an std::ofstream.
	 */
	template<typename ... Args>
	void print(const Args& ... args)
	{
		if (m_disabled)
			return;	//this way, the check is not performed for each argument
		printImpl(args...);
	}

private:
	bool m_disabled;

	template<typename T, typename ... Args>
	void printImpl(const T& data, Args ... args)
	{
		std::cout << data;	//print the data
		print(args...);		//print even more data
	}

	/**
	 * @brief print base case. Finalizes printing the data.
	 */
	template<typename ...>
	void printImpl()
	{
		//nothing to do here
	}
};

}
/* namespace n_tracers */

#endif /* SRC_TRACERS_POLICIES_H_ */