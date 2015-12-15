/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_TRACERS_POLICIES_H_
#define SRC_TRACERS_POLICIES_H_

#include <fstream>
#include <iostream>
#include <assert.h>
#include "tools/objectfactory.h"

namespace n_tracers {

/**
 * @brief Tracer output policy. Output will be printed to a single file.
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
	/**
	 * @brief Destructor. Will release all resources associated with the output file.
	 */
	~FileWriter();

	/**
	 * @brief Constructor for this policy.
	 * @note The policy won't be ready for writing the output until it has been initialized.
	 * @see initialize
	 */
	FileWriter();

	/**
	 * @brief Deleted copy constructor
	 * @note A FileWriter object can't be copied, only moved.
	 */
	FileWriter(const FileWriter& other) = delete;

	/**
	 * @brief move constructor.
	 *
	 * All relevant data will be moved into the new object.
	 */
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
	inline void print(const Args&... args)
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
	inline void printImpl(const T& data, const Args&... args)
	{
		assert(isInitialized());
		*m_stream << data;	//print the data
		printImpl(args...);	//print more data
	}

	/**
	 * @brief print base case. Finalizes printing the data.
	 */
	template<typename ...>
	inline void printImpl()
	{
		//nothing to do here
	}
};

/**
 * @brief Tracer output policy. Output will be printed to multiple files.
 *
 * All filenames generated by this policy will follow this pattern:
 * 	[fileName]_[count][extend]
 * With fileName = "myFile" and extend=".dat", the following filenames are generated:
 * 	myFile_0.dat
 * 	myFile_1.dat
 * 	myFile_2.dat
 * 	myFile_3.dat
 * 	...
 *
 * @note Currently, only the CellTracer fully supports this policy because it is the only tracer for which this policy would make sense.
 */
class MultiFileWriter
{
public:
	/**
	 * @brief Opens the file for output
	 *
	 * @param fileName The name of the file.
	 * @param extend [optional, default: ".txt"] The extension of the file.
	 * @postcondition The output policy is ready for output.
	 *
	 *
	 * All filenames generated by this policy will follow this pattern:
	 * 	[fileName]_[count][extend]
	 * With fileName = "myFile" and extend=".dat", the following filenames are generated:
	 * 	myFile_0.dat
	 * 	myFile_1.dat
	 * 	myFile_2.dat
	 * 	myFile_3.dat
	 * 	...
	 */
	void initialize(const std::string& fileName, std::string extend = ".txt");

	/**
	 * @brief Stops the tracer from generating any further output.
	 * @warning	We assume that this function is not called while a simulation is in progress.
	 * 		If there is, we can make no guarantee when the output generation actually stops.
	 */
	void stopTracer();

	/**
	 * @brief Restarts the tracer.
	 * @throws std::ios_base::failure If the previous file could not be opened for output.
	 */
	void startTracer();

	/**
	 * @brief Checks whether the policy has been properly initialized. That is, whether a filename has been chosen.
	 */
	bool isInitialized() const;

	/**
	 * @brief Starts a new file.
	 * @see initialize for how the filenames are created.
	 */
	void startNewFile();

	/**
	 * @brief Closes the currently opened file
	 */
	void closeFile();

protected:
	/**
	 * @brief Destructor. Will release all resources associated with the output file(s).
	 */
	~MultiFileWriter();

	/**
	 * @brief Constructor for this policy.
	 * @note The policy won't be ready for writing the output until it has been initialized.
	 * @see initialize
	 */
	MultiFileWriter();

	/**
	 * @brief Deleted copy constructor
	 * @note A MultiFileWriter object can't be copied, only moved.
	 */
	MultiFileWriter(const FileWriter& other) = delete;

	/**
	 * @brief move constructor.
	 *
	 * All relevant data will be moved into the new object.
	 */
	MultiFileWriter(MultiFileWriter&& other) = default;

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
	std::ofstream m_stream;
	bool m_disabled;
	std::string m_filename;
	std::string m_fileExtend;
	std::size_t m_fileCount;

	template<typename T, typename ... Args>
	inline void printImpl(const T& data, const Args&... args)
	{
		m_stream << data;	//print the data
		printImpl(args...);	//print more data
	}

	/**
	 * @brief print base case. Finalizes printing the data.
	 */
	template<typename ...>
	inline void printImpl()
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

	/**
	 * @brief Constructor for this policy.
	 *
	 * The policy will be immediately ready for handling the generated output.
	 */
	CoutWriter();

	/**
	 * @brief copy constructor.
	 */
	CoutWriter(const CoutWriter& other);

	/**
	 * @brief move constructor.
	 *
	 * All relevant data will be moved into the new object.
	 */
	CoutWriter(CoutWriter&& other);

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
	inline void print(const Args& ... args)
	{
		if (m_disabled)
			return;	//this way, the check is not performed for each argument
		printImpl(args...);
	}

private:
	bool m_disabled;

	template<typename T, typename ... Args>
	inline void printImpl(const T& data, Args ... args)
	{
		std::cout << data;	//print the data
		print(args...);		//print even more data
	}

	/**
	 * @brief print base case. Finalizes printing the data.
	 */
	template<typename ...>
	inline void printImpl()
	{
		//nothing to do here
	}
};

}
/* namespace n_tracers */

#endif /* SRC_TRACERS_POLICIES_H_ */
