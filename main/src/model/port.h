/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Pieter Lauwers, Matthijs Van Os
 */

#ifndef PORT_H_
#define PORT_H_

#include <vector>
#include <map>
#include <algorithm>
#include "network/message.h"
#include "model/zfunc.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include "model/uuid.h"
#include <set>


#include "tools/statistic.h"
#include "tools/gviz.h"

// Don't include atomicmodel here

/**
 * Registers flag set on revert if at all.
 */
namespace n_tlocal{
        // The underscore postfix is a subtle hint this function is not part of any API we offer.
        inline
        bool & revert_flag__()
        {
                thread_local bool revert_set = false;
                return revert_set;
        }
        
        inline
        void setRevert(bool b){revert_flag__()=b;}
        
        inline
        bool isRevertSet(){return revert_flag__();}
}

namespace n_model {

class Port;

typedef std::shared_ptr<Port> t_portptr;
typedef Port* t_portptr_raw;
typedef std::pair<t_portptr_raw, t_zfunc> t_outconnect;

class Model;

class Port
{
    friend class n_tools::GVizWriter;
private:
	std::string m_name;
	std::string m_hostname;
        std::size_t m_portid;
	bool m_inputPort;

	//vectors for single connections
	std::vector<t_portptr_raw> m_ins;
	std::vector<t_outconnect> m_outs;
	//vectors for direct connect connections
	std::vector<t_portptr_raw> m_coupled_ins;
	std::vector<t_outconnect> m_coupled_outs;
	//vector of recently send messages for the tracer
	std::vector<n_network::t_msgptr> m_sentMessages;
	//vector of recently received messages for the tracer
	std::vector<n_network::t_msgptr> m_receivedMessages;

	bool m_usingDirectConnect;
        
        Model* m_hostmodel;

        // Workaround, port is included in model -> atomicmodel, meaning we need to fwd declare
        // atomicmodel, but createMessages is templated (and header defined) so we can't call 
        // incomplete typed objects there. Soln is to get info from ptr in port.cpp (where we can get at atomicmodel)
        const uuid&
        getModelUUID()const;

        n_network::t_timestamp
        imminentTime()const;

public:
	/**
	 * Constructor of Port
	 *
	 * @param name the local name of the port
	 * @param host pointer to the host of the port
	 * @param inputPort whether or not this is an input port
	 */
	Port(const std::string& name, Model* host, std::size_t portid, bool inputPort);
        
        ~Port();

	/**
	 * Returns the name of the port
	 *
	 * @return local name of the port
	 */
	std::string getName() const;

	/**
	 * Returns the hostname of the port
	 *
	 * @return Hostname of the port
	 */
	std::string getHostName() const;

	/**
	 * @return The port id, which is a unique port identifier
	 * among all input or output ports of a model.
	 */
	inline std::size_t getPortID() const
	{ return m_portid; }

	/**
	 * @brief Sets the port id
	 */
	inline void setPortID(std::size_t id)
	{ m_portid = id; }

	/**
	 * Returns whether or not this is an input port
	 *
	 * @return whether or not this is an input port
	 */
	bool isInPort() const;

	/**
	 * Sets an output port with a matching function to this port
	 *
	 * @param port new output port
	 * @param function function matching with the new output port, when
         *      no function is necessary, use nullptr
	 *
	 * @return whether or not the port wasn't already added
	 */
	bool setZFunc(const t_portptr_raw port, t_zfunc function = nullptr);
        

	/**
	 * Sets an input port to this port
	 *
	 * @param port new input port
	 *
	 * @return whether or not the port was already added
	 */
	bool setInPort(const t_portptr_raw port);

	/**
	 * @brief Removes a port from the list of outgoing connections
	 */
	void removeOutPort(const t_portptr_raw port);

	/**
	 * @brief Removes a port from the list of incoming connections
	 */
	void removeInPort(const t_portptr_raw port);

	/**
	 * @brief Adds an outgoing connection with a Z function.
	 * @param port new output port
	 * @param function function matching with the new output port
	 * @note This functionality is used for direct connect and should not be used for creating regular connections.
	 * @see setZFunc
	 */
	void setZFuncCoupled(const t_portptr_raw port, t_zfunc function = nullptr);

	/**
	 * @brief Adds an incoming connection.
	 * @param port new input port
	 * @note This functionality is used for direct connect and should not be used for creating regular connections.
	 * @see setInPort
	 */
	void setInPortCoupled(const t_portptr_raw port);

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
	const std::vector<t_portptr_raw>& getIns() const
	{
		return m_ins;
	}
	/**
	 * @brief Returns a reference to all ports from outgoing connections
	 */
	const std::vector<t_outconnect>& getOuts() const
	{
		return m_outs;
	}
	/**
	 * @brief Returns a reference to all incoming connections
	 */
	std::vector<t_portptr_raw>& getIns()
	{
		return m_ins;
	}
	/**
	 * @brief Returns a reference to all outgoing connections
	 */
	std::vector<t_outconnect>& getOuts()
	{
		return m_outs;
	}
	/**
	 * @brief Returns a reference to all ports from incoming connections, using the directConnect algorithm.
	 */
	const std::vector<t_portptr_raw>& getCoupledIns() const
	{
		return m_coupled_ins;
	}
	/**
	 * @brief Returns a reference to all ports from outgoing connections, using the directConnect algorithm.
	 */
	const std::vector<t_outconnect>& getCoupledOuts() const
	{
		return m_coupled_outs;
	}

	/**
	 * @brief Removes all incoming and outgoing connections.
	 */
	void clearConnections();

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
	void addMessage(const n_network::t_msgptr& message);

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
	void addInfluencees(std::vector<std::size_t>& influences) const;

	const Model* getHost() const
	{return m_hostmodel;}

	Model* getHost()
	{return m_hostmodel;}

        
//-------------statistics gathering--------------
//#ifdef USE_STAT
private:
	//counters for statistics
	std::map<t_portptr_raw, n_tools::t_uintstat> m_sendstat;
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
        const n_model::uuid& srcuuid = this->getModelUUID();
        const n_network::t_timestamp nowtime = this->imminentTime();
	
#ifndef NO_TRACER
	{
                // This message is simply to allow correct tracing of a model that generates output, but does not send it (ie trafficlight)
		m_sentMessages.push_back(createMsg(
                                srcuuid, uuid(0, 0),nowtime,
                                getPortID(), getPortID(),
                                message, nullptr)
                );// Use dst==src to avoid overflow.
                
	}
#endif
	// We want to iterate over the correct ports (whether we use direct connect or not)
	if (!m_usingDirectConnect) {
		container.reserve(m_outs.size());
		for (const t_outconnect& pair : m_outs) {
			// We now know everything, we create the message, apply the zFunction and push it on the vector
			container.push_back(createMsg(srcuuid, pair.first->getModelUUID(),
				nowtime,
				pair.first->getPortID(), getPortID(),
				message, pair.second));
#ifdef USE_STAT
			++m_sendstat[pair.first];
#endif
		}
	} else {
		container.reserve(container.size() + m_coupled_outs.size());
		for (const t_outconnect& pair : m_coupled_outs) {
			container.push_back(createMsg(srcuuid, pair.first->getModelUUID(),
				nowtime,
				pair.first->getPortID(), getPortID(),
				message, pair.second));
#ifdef USE_STAT
			++m_sendstat[pair.first];
#endif
                        LOG_DEBUG("Message created == ", container.back()->toString());
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

/**
 * @brief Type specific implementation for creating a single message
 * @param dest The name of the destination model
 * @param destP The full name of the destination port
 * @param sourceP The full name of the source port
 * @param msg The data send with this message
 * @param func The ZFunction that must be applied on this message
 */
/**
 * @brief Base case. Used for everything except strings
 */
template<typename DataType>
inline n_network::t_msgptr createMsgImpl(n_model::uuid srcUUID, n_model::uuid dstUUID,
	const n_network::t_timestamp& time_made,
	std::size_t destport, std::size_t sourceport,
        const DataType& msg, t_zfunc func)
{
	n_network::t_msgptr messagetobesend = n_tools::createPooledObject<n_network::SpecializedMessage<DataType>>(srcUUID, dstUUID,
		time_made, destport, sourceport, msg);
	// If the zFunction is defined AKA not equal to nullptr AKA implicitly cast to true
	// Call the zFunction onto the message to be send

	if (func)
		messagetobesend = (*func)(messagetobesend);

	return messagetobesend;
}

/**
 * @brief Overload for std::string
 */
template<>
inline n_network::t_msgptr createMsgImpl<std::string>(n_model::uuid srcUUID, n_model::uuid dstUUID,
	const n_network::t_timestamp& time_made,
	std::size_t destport, std::size_t sourceport,
	const std::string& msg, t_zfunc func)
{
	n_network::t_msgptr messagetobesend =
		n_tools::createPooledObject<n_network::SpecializedMessage<std::string>>(
		srcUUID, dstUUID, time_made, destport, sourceport, msg);
	// If the zFunction is defined AKA not equal to nullptr AKA implicitly cast to true
	// Call the zFunction onto the message to be send
    
	if (func)
		messagetobesend = (*func)(messagetobesend);

	return messagetobesend;
}

/**
 * @brief Overload for string literals
 */
template<>
inline n_network::t_msgptr createMsgImpl<const char*>(n_model::uuid srcUUID, n_model::uuid dstUUID,
	const n_network::t_timestamp& time_made,
	std::size_t destport, std::size_t sourceport,
	const char* const & msg, t_zfunc func)
{
	n_network::t_msgptr messagetobesend =
		n_tools::createPooledObject<n_network::SpecializedMessage<std::string>>(
		srcUUID, dstUUID, time_made, destport, sourceport, msg);
	// If the zFunction is defined AKA not equal to nullptr AKA implicitly cast to true
	// Call the zFunction onto the message to be send

	if (func)
		messagetobesend = (*func)(messagetobesend);

	return messagetobesend;
}


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
inline n_network::t_msgptr createMsg(n_model::uuid srcUUID, n_model::uuid dstUUID,
	const n_network::t_timestamp& time_made,
	std::size_t destport, std::size_t sourceport,
        const T& msg, t_zfunc func)
{

	return createMsgImpl<typename array2ptr<T>::type>(srcUUID, dstUUID, time_made, destport, sourceport, msg, func);
}
}

#endif /* PORT_H_ */
