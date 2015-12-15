/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve
 */

#include "performance/queuenetwork/queuenetwork.h"

namespace n_queuenetwork {


std::size_t MsgGenerator::counter = 0;

std::string n_queuenetwork::MsgGenerator::getNewName()
{
	return "MsgGenerator" + n_tools::toString(counter++);
}

MsgGenerator::MsgGenerator(std::size_t chance,
	QueueMsg::t_size rate,
	QueueMsg::t_size nsize,
	QueueMsg::t_size psize)
	:
	n_model::AtomicModel<GeneratorState>(getNewName(), counter, std::size_t(0u)),
	m_priorityChance(chance),
	m_rate(rate), m_normalSize(nsize), m_prioritySize(psize),
	m_out(addOutPort("out"))
{
	assert((double(nsize)/double(psize)) == double(std::size_t(double(nsize)/double(psize)))
		&& "The size of a normal message must be a multiple of the size of a priority message.");
}

n_model::t_timestamp MsgGenerator::timeAdvance() const
{
	return m_rate;
}

void MsgGenerator::intTransition()
{
	state() = m_rand();
}

void MsgGenerator::extTransition(const std::vector<n_network::t_msgptr>&)
{
	assert(false && "We should never get an external transition in a QueueNetwork Generator.");
}

void MsgGenerator::confTransition(const std::vector<n_network::t_msgptr>&)
{
	state() = m_rand();
}

void MsgGenerator::output(std::vector<n_network::t_msgptr>& msgs) const
{
	static std::uniform_int_distribution<std::size_t> dist(1, 100);
	m_rand.seed(state());
	bool priority = dist(m_rand) <= m_priorityChance;
	QueueMsg::t_size size = priority? m_prioritySize:m_normalSize;
	m_out->createMessages(QueueMsg(size, priority), msgs);
}

n_network::t_timestamp MsgGenerator::lookAhead() const
{
	return m_rate;
}

Server::Server(QueueMsg::t_size rate, double maxSize):
	n_model::AtomicModel<ServerState>("Server"),
	m_rate(rate), m_maxSize(maxSize),
	m_out(addOutPort("out")), m_in(addInPort("in"))
{
}

n_model::t_timestamp Server::timeAdvance() const
{
	if(state().m_priorityMsgs.size())
		return state().m_priorityMsgs.front().m_size;
	if(state().m_msgs.size())
		return state().m_msgs.front().m_size;
	return n_model::t_timestamp::infinity();
}

void Server::intTransition()
{
	ServerState& st = state();
	double i = 0;
	auto piter = st.m_priorityMsgs.begin();
	auto niter = st.m_msgs.begin();
	double size = m_maxSize*getTimeElapsed().getTime();
	LOG_DEBUG("applicable size: ", size, ", elapsed time: ", getTimeElapsed());
	while(true) {
		if(piter != st.m_priorityMsgs.end()){
			if(i + piter->m_size <= size){
				piter = st.m_priorityMsgs.erase(piter);
				i += piter->m_size;
				continue;
			}
			break;
		} else if(niter != st.m_msgs.end()){
			if(i + niter->m_size <= size){
				niter = st.m_msgs.erase(niter);
				i += niter->m_size;
				continue;
			}
			break;
		}
		break;
	}
}

void Server::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	ServerState& st = state();
	for(const n_network::t_msgptr& ptr: message){
		const QueueMsg& msg = n_network::getMsgPayload<QueueMsg>(ptr);
		if(msg.m_isPriority)
			st.m_priorityMsgs.push_back(msg);
		else
			st.m_msgs.push_back(msg);
	}
}

void Server::confTransition(const std::vector<n_network::t_msgptr>& message)
{
	ServerState& st = state();
	double i = 0;
	auto piter = st.m_priorityMsgs.begin();
	auto niter = st.m_msgs.begin();
	double size = m_maxSize*getTimeElapsed().getTime();
	LOG_DEBUG("applicable size: ", size, ", elapsed time: ", getTimeElapsed());
	while(true) {
		if(piter != st.m_priorityMsgs.end()){
			if(i + piter->m_size <= size){
				piter = st.m_priorityMsgs.erase(piter);
				i += piter->m_size;
				continue;
			}
			break;
		} else if(niter != st.m_msgs.end()){
			if(i + niter->m_size <= size){
				niter = st.m_msgs.erase(niter);
				i += niter->m_size;
				continue;
			}
			break;
		}
		break;
	}
	for(const n_network::t_msgptr& ptr: message){
		const QueueMsg& msg = n_network::getMsgPayload<QueueMsg>(ptr);
		if(msg.m_isPriority)
			st.m_priorityMsgs.push_back(msg);
		else
			st.m_msgs.push_back(msg);
	}
}

void Server::output(std::vector<n_network::t_msgptr>& msgs) const
{
	const ServerState& st = state();
	double i = 0;
	auto piter = st.m_priorityMsgs.begin();
	auto niter = st.m_msgs.begin();
	double size = m_maxSize*getTimeElapsed().getTime();
	LOG_DEBUG("applicable size: ", size, ", elapsed time: ", getTimeElapsed());
	while(true) {
		if(piter != st.m_priorityMsgs.end()){
			if(i + piter->m_size <= size){
				m_out->createMessages(QueueMsg(*piter), msgs);
				++piter;
				i += piter->m_size;
				continue;
			}
			break;
		}
		if(niter != st.m_msgs.end()){
			if(i + niter->m_size <= size){
				m_out->createMessages(QueueMsg(*niter), msgs);
				++niter;
				i += niter->m_size;
				continue;
			}
			break;
		}
		break;
	}
}

n_network::t_timestamp Server::lookAhead() const
{
	return m_rate;
}

Splitter::Splitter(std::size_t size, QueueMsg::t_size rate)
	:n_model::AtomicModel<SplitterState>("Splitter"), m_rate(rate),
	 m_size(size), m_dist(0, m_size-1),
	 m_in(addInPort("in"))
{
	state().m_timeleft = rate;
	for(std::size_t i = 0; i < m_size; ++i)
		addOutPort("[" + n_tools::toString(i) + "]");
}

n_model::t_timestamp Splitter::timeAdvance() const
{
	LOG_DEBUG("last time: ", getTimeLast());
	return state().m_msgs.size()? state().m_timeleft: n_network::t_timestamp::infinity();
}

void Splitter::intTransition()
{
	SplitterState& st = state();
	st.m_timeleft = m_rate;
	st.m_msgs.clear();
	st.seed = m_rand();
}

void Splitter::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	SplitterState& st = state();
	if(st.m_msgs.size())
		st.m_timeleft -= getTimeElapsed().getTime();
	else
		st.m_timeleft = m_rate;
	st.seed = m_rand();
	st.m_msgs.reserve(st.m_msgs.size() + message.size());
	for(const n_network::t_msgptr& ptr: message){
		const QueueMsg& msg = n_network::getMsgPayload<QueueMsg>(ptr);
		st.m_msgs.push_back(msg);
	}
}

void Splitter::confTransition(const std::vector<n_network::t_msgptr>& message)
{
	SplitterState& st = state();
	st.m_timeleft = m_rate;
	st.m_msgs.clear();
	st.seed = m_rand();
	st.m_msgs.reserve(message.size());
	for(const n_network::t_msgptr& ptr: message){
		const QueueMsg& msg = n_network::getMsgPayload<QueueMsg>(ptr);
		st.m_msgs.push_back(msg);
	}
}

void Splitter::output(std::vector<n_network::t_msgptr>& msgs) const
{
	const SplitterState& st = state();
	m_rand.seed(st.seed);
	for(const QueueMsg& qmsg: st.m_msgs){
		std::size_t port = m_dist(m_rand);
		m_oPorts[port]->createMessages(qmsg, msgs);
	}
}

n_network::t_timestamp n_queuenetwork::Splitter::lookAhead() const
{
	return S_PRIORITY_SIZE;
}

SingleServerNetwork::SingleServerNetwork(std::size_t numGenerators, std::size_t splitterSize,
        std::size_t priorityChance, QueueMsg::t_size rate, QueueMsg::t_size nsize, QueueMsg::t_size psize)
	: n_model::CoupledModel("SingleServerNetwork")
{
	QueueMsg::t_size k = numGenerators*nsize;
	LOG_DEBUG("server network k = ", k);
	n_model::t_modelptr server = n_tools::createObject<Server>(rate, k);
	n_model::t_modelptr splitter = n_tools::createObject<Splitter>(splitterSize, nsize);
	n_model::t_modelptr receiver = n_tools::createObject<Receiver>();
	addSubModel(server);
	addSubModel(splitter);
	addSubModel(receiver);
	for(std::size_t i = 0; i < numGenerators; ++i){
		n_model::t_modelptr gen = n_tools::createObject<MsgGenerator>(priorityChance, rate, nsize, psize);
		addSubModel(gen);
		connectPorts(gen->getOPorts()[0], server->getIPorts()[0]);
	}
	connectPorts(server->getOPorts()[0], splitter->getIPorts()[0]);
	for(auto& ptr: splitter->getOPorts())
		connectPorts(ptr, receiver->getIPorts()[0]);
}

FeedbackServerNetwork::FeedbackServerNetwork(std::size_t numGenerators,
        std::size_t priorityChance, QueueMsg::t_size rate, QueueMsg::t_size nsize, QueueMsg::t_size psize)
	: n_model::CoupledModel("FeedbackServerNetwork")
{
	QueueMsg::t_size k = numGenerators*nsize;
	n_model::t_modelptr server = n_tools::createObject<Server>(rate, k);
	n_model::t_modelptr splitter = n_tools::createObject<Splitter>(numGenerators, nsize);
	addSubModel(server);
	addSubModel(splitter);
	const auto& splitterouts = splitter->getOPorts();
	connectPorts(server->getOPorts()[0], splitter->getIPorts()[0]);
	for(std::size_t i = 0; i < numGenerators; ++i){
		n_model::t_modelptr gen = n_tools::createObject<GenReceiver>(priorityChance, rate, nsize, psize);
		addSubModel(gen);
		connectPorts(gen->getOPorts()[0], server->getIPorts()[0]);
		connectPorts(splitterouts[i], gen->getIPorts()[0]);
	}
}

} /* n_queuenetwork */
