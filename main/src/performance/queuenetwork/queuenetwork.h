/*
 * queuenetwork.h
 *
 *  Created on: Sep 25, 2015
 *      Author: DEVS Ex Machina
 */

#ifndef SRC_PERFORMANCE_QUEUENETWORK_QUEUENETWORK_H_
#define SRC_PERFORMANCE_QUEUENETWORK_QUEUENETWORK_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"


#define T_GEN_TA 100
#define S_NORMAL_SIZE 100
#define S_PRIORITY_SIZE 5

namespace n_queuenetwork {

struct QueueMsg
{
	typedef n_network::t_timestamp::t_time t_size;
	t_size m_size;
	t_size m_startsize;
	bool m_isPriority;

	QueueMsg(t_size size, bool priority = false)
		: m_size(size), m_startsize(size), m_isPriority(priority)
	{}

	QueueMsg(const QueueMsg& other)
		: QueueMsg(other.m_startsize, other.m_isPriority)
	{}
};

inline
std::ostream& operator<<(std::ostream& out, const QueueMsg& msg)
{
	out << msg.m_size;
	if(msg.m_isPriority)
		out << "[P]";
	return out;
}

typedef std::size_t GeneratorState;	//used as seed for the random number generator.
typedef std::mt19937_64 t_randgen;

/**
 * Generates messages at a constant rate.
 */
class MsgGenerator: public n_model::AtomicModel<GeneratorState>
{
private:
	static std::string getNewName();
	static std::size_t counter;

	std::size_t m_priorityChance;
	n_model::t_timestamp m_rate;
	QueueMsg::t_size m_normalSize;
	QueueMsg::t_size m_prioritySize;
	mutable t_randgen m_rand;
public:
	n_model::t_portptr m_out;

	MsgGenerator(std::size_t chance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE);
	virtual ~MsgGenerator(){}

	virtual n_model::t_timestamp timeAdvance() const override;
	virtual void intTransition() override;
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
	virtual n_network::t_timestamp lookAhead() const override;

};

class GenReceiver: public MsgGenerator
{
public:
	n_model::t_portptr m_in;
	GenReceiver(std::size_t chance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE)
	: MsgGenerator(chance, rate, nsize, psize), m_in(addInPort("in"))
	{}
	virtual ~GenReceiver(){}
};

/**
 * Simply ignores any messages it gets
 */
class Receiver: public n_model::AtomicModel<void>
{
public:
	n_model::t_portptr m_in;
	Receiver(): n_model::AtomicModel<void>("receiver"), m_in(addInPort("in"))
	{}
	virtual ~Receiver(){}

	virtual n_model::t_timestamp timeAdvance() const override
		{ return n_network::t_timestamp::infinity(); }
	virtual n_network::t_timestamp lookAhead() const override
		{ return S_PRIORITY_SIZE; }

	virtual void intTransition() override{}
	virtual void extTransition(const std::vector<n_network::t_msgptr> &) override{}
	virtual void confTransition(const std::vector<n_network::t_msgptr> &) override{}
	virtual void output(std::vector<n_network::t_msgptr>&)const override{}
};

struct SplitterState
{
	std::vector<QueueMsg> m_msgs;
	std::size_t seed;
	QueueMsg::t_size m_timeleft;

	SplitterState(): seed(0), m_timeleft(0)
	{}
};

class Splitter: public n_model::AtomicModel<SplitterState>
{
private:
	QueueMsg::t_size m_rate;
	std::size_t m_size;
	mutable std::uniform_int_distribution<std::size_t> m_dist;
	mutable t_randgen m_rand;
public:
	n_model::t_portptr m_in;

	Splitter(std::size_t size, QueueMsg::t_size rate = S_NORMAL_SIZE);
	virtual ~Splitter(){}

	virtual n_model::t_timestamp timeAdvance() const override;
	virtual void intTransition() override;
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
	virtual n_network::t_timestamp lookAhead() const override;
};

struct ServerState
{
	std::deque<QueueMsg> m_msgs;
	std::deque<QueueMsg> m_priorityMsgs;
};

class Server: public n_model::AtomicModel<ServerState>
{
private:
	n_network::t_timestamp m_rate;	//generator rate
	double m_maxSize;
public:
	n_model::t_portptr m_out;
	n_model::t_portptr m_in;
	Server(QueueMsg::t_size rate, double maxSize = 100);
	virtual ~Server(){}

	virtual n_model::t_timestamp timeAdvance() const override;
	virtual void intTransition() override;
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
	virtual n_network::t_timestamp lookAhead() const override;

};

class SingleServerNetwork: public n_model::CoupledModel
{
public:
	SingleServerNetwork(std::size_t numGenerators, std::size_t splitterSize, std::size_t priorityChance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE);
	virtual ~SingleServerNetwork(){}
};

class FeedbackServerNetwork: public n_model::CoupledModel
{
public:
	FeedbackServerNetwork(std::size_t numGenerators, std::size_t priorityChance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE);
	virtual ~FeedbackServerNetwork(){}
};

class QueueAlloc: public n_control::Allocator
{
private:
	std::size_t m_counter;
public:
	QueueAlloc(): m_counter(0){

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr& ptr){
		auto p = std::dynamic_pointer_cast<n_queuenetwork::MsgGenerator>(ptr);
		if(p == nullptr)
			return 0;
		if(coreAmount() < 2)
			return 0;
		if(coreAmount() == 2)
			return 1;
		std::size_t res = 1 + (m_counter++)%(coreAmount()-1);
		if(res >= coreAmount())
			res = coreAmount()-1;
		LOG_INFO("Putting model ", ptr->getName(), " in core ", res, " out of ", coreAmount());
		return res;
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		for(const n_model::t_atomicmodelptr& ptr: models)
			ptr->setCorenumber(allocate(ptr));
	}
};

} /* namespace n_queuenetwork */


template<>
struct ToString<n_queuenetwork::SplitterState>
{
	static std::string exec(const n_queuenetwork::SplitterState& s){
		return "splitting: " + n_tools::toString(s.m_msgs.size()) + " after time: " + n_tools::toString(s.m_timeleft);
	}
};
template<>
struct ToString<n_queuenetwork::ServerState>
{
	static std::string exec(const n_queuenetwork::ServerState& s){
		return "prioritized msgs: " + n_tools::toString(s.m_priorityMsgs.size()) + ", other msgs: " + n_tools::toString(s.m_msgs.size());
	}
};

#endif /* SRC_PERFORMANCE_QUEUENETWORK_QUEUENETWORK_H_ */
