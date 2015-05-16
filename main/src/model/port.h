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
#include "message.h"
#include "zfunc.h"
#include "globallog.h"
#include "objectfactory.h"

namespace n_model {

class Port;

typedef std::shared_ptr<Port> t_portptr;

class Port
{
private:
	std::string m_name;
	std::string m_hostname;
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
	void clear();

	/**
	 * Sets a coupled input port to this port (for direct connect usage)
	 *
	 * @param port New input port
	 *
	 */
	void removeOutPort(const t_portptr& port);
	void removeInPort(const t_portptr& port);

	void setZFuncCoupled(const t_portptr& port, t_zfunc function);

	void setInPortCoupled(const t_portptr& port);

	/*
	 * Sets the if whether the port is currently using direct connect or not
	 *
	 * @param dc True or false, depending of the port is currently using direct connect
	 */
	void setUsingDirectConnect(bool dc);

	void resetDirectConnect();
	bool isUsingDirectConnect() const;


	/**
	 * Function that creates messages with a give payload.
	 * These messages are addressed to all out-ports that are currently connected
	 * Note that these out-ports can differ if you are using direct connect!
	 * Zfunctions that apply will be called upon the messages
	 *
	 * @param message The payload of the message that is to be sent
	 */
	std::vector<n_network::t_msgptr> createMessages(std::string message);

	/**
	 * @brief Creates messages with a given payload and stores them in a container
	 * These messages are addressed to all out-ports that are currently connected
	 * Zfunctions that apply will be called on the messages
	 *
	 * @param message The payload of the message that is to be sent
	 * @param container A reference to the container in which the messages will be stored
	 *
	 * @return For convenience, the argument passed as the container parameter is returned.
	 *
	 * @note These out-ports can differ if you are using direct connect
	 */
	std::vector<n_network::t_msgptr> createMessages(std::string message, std::vector<n_network::t_msgptr>& container);

	const std::vector<t_portptr>& getIns() const;
	const std::map<t_portptr, t_zfunc>& getOuts() const;
	std::vector<t_portptr>& getIns();
	std::map<t_portptr, t_zfunc>& getOuts();
	const std::vector<t_portptr>& getCoupledIns() const;
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
};

}

#endif /* PORT_H_ */
