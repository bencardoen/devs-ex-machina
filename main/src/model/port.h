/*
 * port.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Stijn
 */

#ifndef PORT_H_
#define PORT_H_

#include "serialization/archive.h"
#include <vector>
#include <map>
#include <algorithm>
#include "network/message.h"
#include "model/zfunc.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include <set>


#include "tools/statistic.h"

class TestCereal;

namespace n_model {

class Port;

typedef std::shared_ptr<Port> t_portptr;

class Port
{
	friend class ::TestCereal;
private:
	std::string m_name;
	std::string m_hostname;
        std::string m_fullname;
	bool m_inputPort;

	std::vector<t_portptr> m_ins;
	std::map<t_portptr, t_zfunc> m_outs;

	std::map<t_portptr, std::vector<t_zfunc>> m_coupled_outs;
	std::vector<t_portptr> m_coupled_ins;

	/**
	 * Vectors of recently send messages for the tracer
	 */
	std::vector<n_network::t_msgptr> m_sentMessages;

	/**
	 * Vectors of recently received messages for the tracer
	 */
	std::vector<n_network::t_msgptr> m_receivedMessages;

	bool m_usingDirectConnect;

public:
	/**
	 * Constructor of Port
	 *
	 * @param name the local name of the port
	 * @param host pointer to the host of the port
	 * @param inputPort whether or not this is an input port
	 */
	Port(std::string name, std::string hostname, bool inputPort);

	/**
	 * Returns the name of the port
	 *
	 * @return local name of the port
	 */
	std::string getName() const;

	/**
	 * Returns the complete name of the port
	 *
	 * @return fully qualified name of the port
	 */
	std::string getFullName() const;

	/**
	 * Returns the hostname of the port
	 *
	 * @return Hostname of the port
	 */
	std::string getHostName() const;

	/**
	 * Returns whether or not this is an input port
	 *
	 * @return whether or not this is an input port
	 */
	bool isInPort() const;

	/**
	 * Returns the function matching with the given port.
	 *
	 * @return the matching function
	 */
	t_zfunc getZFunc(const t_portptr& port) const;

	/**
	 * Sets an output port with a matching function to this port
	 *
	 * @param port new output port
	 * @param function function matching with the new output port
	 *
	 * @return whether or not the port wasn't already added
	 */
	bool setZFunc(const t_portptr& port, t_zfunc function);

	/**
	 * Sets an input port to this port
	 *
	 * @param port new input port
	 *
	 * @return whether or not the port was already added
	 */
	bool setInPort(const t_portptr& port);

	/**
	 * @brief Clears all links to other ports from this message.
	 * @note This function will not remove these references from the other ports.
	 * @see CoupledModel::disconnectPorts to safely remove connections between ports.
	 */
	void clear();

	/**
	 * @brief Removes a port from the list of outgoing connections
	 */
	void removeOutPort(const t_portptr& port);

	/**
	 * @brief Removes a port from the list of incoming connections
	 */
	void removeInPort(const t_portptr& port);

	/**
	 * @brief Adds an outgoing connection with a Z function.
	 * @param port new output port
	 * @param function function matching with the new output port
	 * @note This functionality is used for direct connect and should not be used for creating regular connections.
	 * @see setZFunc
	 */
	void setZFuncCoupled(const t_portptr& port, t_zfunc function);

	/**
	 * @brief Adds an incoming connection.
	 * @param port new input port
	 * @note This functionality is used for direct connect and should not be used for creating regular connections.
	 * @see setInPort
	 */
	void setInPortCoupled(const t_portptr& port);

	/*
	 * Sets the if whether the port is currently using direct connect or not
	 *
	 * @param dc True or false, depending of the port is currently using direct connect
	 */
	void setUsingDirectConnect(bool dc);

	/**
	 * @brief Resets all information pertaining to the direct connect algorithm.
	 */
	void resetDirectConnect();

	/**
	 * @brief Whether or not directConnect is used for this port
	 * @see setUsingDirectConnect
	 */
	bool isUsingDirectConnect() const;

	/**
	 * @brief Creates messages with a given payload and stores them in a container.
	 *
	 * These messages are addressed to all out-ports that are currently connected.
	 * Zfunctions that apply will be called on the messages
	 *
	 * @param message The payload of the message that is to be sent
	 * @param container A reference to the container in which the messages will be stored
	 *
	 * @return For convenience, the argument passed as the container parameter is returned.
	 *
	 * @note These out-ports can differ if you are using direct connect
	 */
	template<typename DataType = std::string>
	void createMessages(const DataType& message, std::vector<n_network::t_msgptr>& container);

	/**
	 * @brief Returns a reference to all ports from incoming connections
	 */
	const std::vector<t_portptr>& getIns() const;
	/**
	 * @brief Returns a reference to all ports from outgoing connections
	 */
	const std::map<t_portptr, t_zfunc>& getOuts() const;
	/**
	 * @brief Returns all ports from incoming connections
	 */
	std::vector<t_portptr>& getIns();
	/**
	 * @brief Returns to all ports from outgoing connections
	 */
	std::map<t_portptr, t_zfunc>& getOuts();
	/**
	 * @brief Returns a reference to all ports from incoming connections, using the directConnect algorithm.
	 */
	const std::vector<t_portptr>& getCoupledIns() const;
	/**
	 * @brief Returns a reference to all ports from outgoing connections, using the directConnect algorithm.
	 */
	const std::map<t_portptr, std::vector<t_zfunc> >& getCoupledOuts() const;

	/**
	 * Clears all sent messages for the tracer
	 */
	void clearSentMessages();

	/**
	 * Clears all received messages for the tracer
	 */
	void clearReceivedMessages();

	/**
	 * Adds message for tracer
	 *
	 * @param message The message that is to be added
	 * @param received If this port received the message or didn't (and thus sent it)
	 */
	void addMessage(const n_network::t_msgptr& message, bool received);

	/**
	 * Get the sent messages (for the tracer)
	 *
	 * @return a vector with all the sent messages
	 */
	const std::vector<n_network::t_msgptr>& getSentMessages() const;

	/**
	 * Get the received messages (for the tracer)
	 *
	 * @return a vector with all the received messages
	 */
	const std::vector<n_network::t_msgptr>& getReceivedMessages() const;

	/**
	 * Adds all this port's influences to the given set.
	 * This function is used in conservative parallel simulation.
	 *
	 * @param influences A set of all current influences (strings: host names) that will be completed
	 */
	void addInfluencees(std::set<std::string>& influences) const;

//-------------serialization---------------------
	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Port>& construct);

//-------------statistics gathering--------------
//#ifdef USESTAT
private:
	//counters for statistics
	std::map<t_portptr, n_tools::t_uintstat> m_sendstat;
public:
	/**
	 * @brief Prints some basic stats.
	 * @param out The output will be printed to this stream.
	 */
	inline void printStats(std::ostream& out = std::cout) const
	{
		for(const auto& i: m_sendstat)
			out << i.second;
	}
//#endif
};


template<typename DataType>
void Port::createMessages(const DataType& message,
        std::vector<n_network::t_msgptr>& container)
{
	const std::string& sourcePort = this->getFullName();
	{
		std::string str = "";
		t_zfunc zfunc = n_tools::createObject<ZFunc>();
		n_network::t_msgptr msg = createMsg("", "", sourcePort, message, zfunc);
//			n_tools::createObject<n_network::Message>("",
//			        n_network::t_timestamp::infinity(), "", sourcePort, message);
		m_sentMessages.push_back(msg);
	}

	// We want to iterate over the correct ports (whether we use direct connect or not)
	if (!m_usingDirectConnect) {
		container.reserve(m_outs.size());
		for (auto& pair : m_outs) {
			t_zfunc& zFunction = pair.second;
			std::string model_destination = pair.first->getHostName();
			//			std::string sourcePort = this->getFullName();
			std::string destPort = n_tools::copyString(pair.first->getFullName());
			n_network::t_timestamp dummytimestamp(n_network::t_timestamp::infinity());

			// We now know everything, we create the message, apply the zFunction and push it on the vector
			container.push_back(createMsg(model_destination, destPort, sourcePort, message, zFunction));
		}
	} else {
		container.reserve(m_coupled_outs.size());
		for (auto& pair : m_coupled_outs) {
			std::string model_destination = pair.first->getHostName();
			//			std::string sourcePort = this->getFullName();
			std::string destPort = n_tools::copyString(pair.first->getFullName());
			for (t_zfunc& zFunction : pair.second) {
				container.push_back(
				        createMsg(model_destination, destPort, sourcePort, message, zFunction));
//#ifdef USESTAT
				++m_sendstat[pair.first];
//#endif
			}
		}
	}
}

/*
 * unnamed namespace to hide the template implementation from the rest of the code
 */
namespace
{

/**
 * @brief Struct for forcing array to pointer type degeneration.
 */
///@{
template<class T>
struct array2ptr
{
    typedef T type;
};

template<class T, std::size_t N>
struct array2ptr<T[N]>
{
    typedef const T * type;
};
///@}

/**
 * @brief Type specific implementation for creating a single message
 * @param dest The name of the destination model
 * @param destP The full name of the destination port
 * @param sourceP The full name of the source port
 * @param msg The data send with this message
 * @param func The ZFunction that must be applied on this message
 */
///@{
/**
 * @brief Base case. Used for everything except strings
 */
template<typename DataType>
inline n_network::t_msgptr createMsgImpl(const std::string& dest, const std::string& destP, const std::string& sourceP,
        const DataType& msg, t_zfunc& func)
{
	n_network::t_msgptr messagetobesend = n_tools::createObject<n_network::SpecializedMessage<DataType>>(dest,
	        n_network::t_timestamp::infinity(), destP, sourceP, msg);
	messagetobesend = (*func)(messagetobesend);
	return messagetobesend;
}

/**
 * @brief Overload for std::string
 */
template<>
inline n_network::t_msgptr createMsgImpl<std::string>(const std::string& dest, const std::string& destP,
        const std::string& sourceP, const std::string& msg, t_zfunc& func)
{
	n_network::t_msgptr messagetobesend = n_tools::createObject<n_network::Message>(dest,
		n_network::t_timestamp::infinity(), destP, sourceP, msg);
	messagetobesend = (*func)(messagetobesend);
	return messagetobesend;
}

/**
 * @brief Overload for string literals
 */
template<>
inline n_network::t_msgptr createMsgImpl<const char*>(const std::string& dest, const std::string& destP,
        const std::string& sourceP, const char* const & msg, t_zfunc& func)
{
	n_network::t_msgptr messagetobesend = n_tools::createObject<n_network::Message>(dest,
		n_network::t_timestamp::infinity(), destP, sourceP, msg);
	messagetobesend = (*func)(messagetobesend);
	return messagetobesend;
}
///@}

}

/**
 * @brief Creating a single message.
 * @tparam T The type of the data that will be contained within the message. This type can usually be inferred from the function arguments.
 * @param dest The name of the destination model
 * @param destP The full name of the destination port
 * @param sourceP The full name of the source port
 * @param msg The data send with this message
 * @param func The ZFunction that must be applied on this message
 */
template<typename T>
inline n_network::t_msgptr createMsg(const std::string& dest, const std::string& destP, const std::string& sourceP,
        const T& msg, t_zfunc& func)
{
	return createMsgImpl<typename array2ptr<T>::type>(dest, destP, sourceP, msg, func);
}
}

#endif /* PORT_H_ */
