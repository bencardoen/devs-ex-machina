/*
 * message.h
 *
 *  Created on: 12 Mar 2015
 *      Author: ben
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

namespace n_network{

class Message{
private:
	const std::string	m_destination_model;
	std::size_t		m_destination_core;
public:
	Message(std::string modeldest, std::size_t coredest=0):m_destination_model(modeldest), m_destination_core(coredest){;}
	std::size_t

	getDestinationCore()const{return m_destination_core;}

	void
	setCore(std::size_t dest){m_destination_core = dest;}	// todo get LookupTable

	virtual ~Message(){;}
};

typedef std::shared_ptr<Message> t_msgptr;

} // end namespace


#endif /* SRC_NETWORK_MESSAGE_H_ */
