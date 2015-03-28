/*
 * tracemessage.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_TRACEMESSAGE_H_
#define SRC_TRACERS_TRACEMESSAGE_H_

#include <functional>	//defines std::less<T>
#include <message.h>

namespace n_tracers {

/**
 * @brief Small message for scheduling the tracer output.
 * @note  The code will probably change a lot
 */
class TraceMessage: public n_network::Message
{
public:
	typedef std::function<void()> t_messagefunc;
	/**
	 * @brief Constructor for the TraceMessage
	 * @param time The timestamp of the message.
	 * 		This is the time that the scheduler uses for scheduling the message
	 * @param func A function object. This function will be executed when the message is received.
	 * 		The function must take no arguments and return no result.
	 */
	TraceMessage(n_network::t_timestamp time, const t_messagefunc& func);

	/**
	 * @brief Executes the scheduled functionality.
	 */
	void execute();

private:
	t_messagefunc m_func;	//function to be executed. This function takes no arguments
};

typedef TraceMessage* t_tracemessageptr;

void scheduleMessage(t_tracemessageptr message);
void traceUntil(n_network::t_timestamp time);
void revertTo(n_network::t_timestamp time);
void clearAll();


/**
 * Entry for a TraceMessage in a scheduler.
 * Keeps modelname and imminent time for a Model, without having to store the entire model.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class TraceMessageEntry
{
	t_tracemessageptr m_pointer;
public:
	t_tracemessageptr getPointer() const
	{
		return m_pointer;
	}

	TraceMessageEntry(t_tracemessageptr ptr): m_pointer(ptr) {}
	~TraceMessageEntry() = default;
	TraceMessageEntry(const TraceMessageEntry&) = default;
	TraceMessageEntry(TraceMessageEntry&&) = default;
	TraceMessageEntry& operator=(const TraceMessageEntry&) = default;
	TraceMessageEntry& operator=(TraceMessageEntry&&) = default;
	TraceMessage& operator*() {
		return *m_pointer;
	}
	const TraceMessage& operator*() const {
		return *m_pointer;
	}
	TraceMessage* operator->() {
		return m_pointer;
	}
	const TraceMessage* operator->() const {
		return m_pointer;
	}

	friend
	bool operator<(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
	{
		if (lhs->getTimeStamp() ==  rhs->getTimeStamp())
			return lhs.m_pointer > rhs.m_pointer;
		return lhs->getTimeStamp() >  rhs->getTimeStamp();
	}

	friend
	bool operator>(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
	{
		return (rhs > lhs);
	}

	friend
	bool operator>=(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
	{
		return (!(lhs < rhs));
	}

	friend
	bool operator==(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
	{
		return (lhs.m_pointer == rhs.m_pointer); // uncomment to allow multiple entries per model
	}

	friend
	std::ostream& operator<<(std::ostream& os, const TraceMessageEntry& rhs){
		return (os<< "Trace message scheduled at " << rhs->getTimeStamp());
	}
};

} /* namespace n_tracers */

namespace std {
template<>
struct hash<n_tracers::TraceMessageEntry>
{
	size_t operator()(const n_tracers::TraceMessageEntry& item) const
	{
		//std::cout << "Hash function for "<< item.getName()<<std::endl;
		return hash<n_tracers::t_tracemessageptr>()(item.getPointer());
	}
};
}
//namespace std {
//template<>
//struct less<n_tracers::t_tracemessageptr>
//{
//	bool operator()(const n_tracers::t_tracemessageptr& k1, const n_tracers::t_tracemessageptr& k2) const
//	{
//		//get the timestamp from the messages and compare those, else: directly compare pointers
//		return(k1->getTimeStamp() < k2->getTimeStamp() || k1 < k2);
//	}
//};
//}

#endif /* SRC_TRACERS_TRACEMESSAGE_H_ */
