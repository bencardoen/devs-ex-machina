/*
 * virustracer.cpp
 *
 *  Created on: May 29, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "virustracer.h"

using namespace n_virus;

std::size_t CellData::m_counter = 0;

n_virus::CellData::CellData(std::string modelName, int value, int production):
	m_modelName(modelName),
	m_value(value),
	m_production(production),
	m_dotName(modelName)
{

}

std::string val2color(int val){
	if(val > 0)
		return "blue";
	else if (val == 0)
		return "gray";
	else
		return "red";
}

std::ostream& n_virus::operator <<(std::ostream& out, const CellData& data)
{
	out << data.m_dotName << "[shape=" << (data.m_dotName[0] == 'C'? "circle": "doublecircle") <<", "
		"color=" << val2color(data.m_value) << ", "
		"fontcolor=" << val2color(data.m_value) << ", "
		"style=bold, "
		;
	if(data.m_value != 0)
		out << "label=<<FONT POINT-SIZE=\"" << int(10+std::sqrt(std::abs(data.m_value))) << "\">" << std::abs(data.m_value) << "</FONT><BR/><FONT POINT-SIZE=\"8\">+" << std::abs(data.m_production) << "</FONT>>";
	else
		out << "label=\"\"";

	out << "];";
	return out;
}

n_virus::MovementData::MovementData(std::string from, std::string to, int amountL, int amountR):
	m_from(from), m_to(to), m_amountLeft(amountL), m_amountRight(amountR)
{
}

std::ostream& n_virus::operator <<(std::ostream& out, const MovementData& data)
{
	if(data.m_amountLeft == 0 && data.m_amountRight == 0){
		out << data.m_to << " -> " << data.m_from << "["
			"color=gray, "
			"fontcolor=gray, "
			"label=\"\", dir=none];"
		;
		return out;
	}
	if(data.m_amountLeft != 0) {
		out << data.m_from << " -> " << data.m_to << "["
			"color=" << val2color(data.m_amountLeft) << ", "
			"fontcolor=" << val2color(data.m_amountLeft) << ", "
			"label=\"" << std::abs(data.m_amountLeft) << "\"];"
		;
	}
	if(data.m_amountRight != 0) {
		out << data.m_to << " -> " << data.m_from << "["
			"color=" << val2color(data.m_amountRight) << ", "
			"fontcolor=" << val2color(data.m_amountRight) << ", "
			"label=\"" << std::abs(data.m_amountRight) << "\"];"
		;
	}
	return out;
}

void n_virus::VirusTracer::traceCall(const n_model::t_atomicmodelptr& adevs, std::size_t coreid, t_timestamp time,
        bool isInit)
{
	LOG_DEBUG("VirusTracer created a message at time", time);

	auto celldevs = std::dynamic_pointer_cast<n_virus::Cell>(adevs);
	if(!celldevs) return;	//don't celltrace models that don't have the correct type of state

	std::string from = celldevs->getName();
	int cellvalue = std::dynamic_pointer_cast<n_virus::CellState>(celldevs->getState())->m_capacity;
	int cellProd = std::dynamic_pointer_cast<n_virus::CellState>(celldevs->getState())->m_production;

	std::vector<MovementData> movements;
	if(isInit) {
		LOG_DEBUG("creating init message: from ", from, " value ", cellvalue);
		for(const auto& pIt: adevs->getOPorts()) {
			for(const auto& port: pIt.second->getCoupledOuts()){
				std::string to = port.first->getHostName();
				if(to.empty())
					continue;

				movements.push_back(MovementData(from, to, 0, 0));
			}
		}
	} else {
		std::string to = from;
		for(const auto& p: adevs->getIPorts()){
			for(const auto& m: p.second->getReceivedMessages()){
				std::string realFrom = n_network::getMsgPayload<MsgData>(m).m_from;
				int value = n_network::getMsgPayload<MsgData>(m).m_value;
				LOG_DEBUG("creating movement message: from ", realFrom, " to ", to, " value ", value);
				movements.push_back(MovementData(realFrom, to, value, 0));
			}
		}
	}

	std::function<void()>  fun = std::bind(&VirusTracer::transitionTrace, this, time, movements, from, cellvalue, cellProd);

	t_tracemessageptr message = n_tools::createRawObject<TraceMessage>(time, fun, coreid);
	//deal with the message
	scheduleMessage(message);
}

void n_virus::VirusTracer::actualTrace(t_timestamp time)
{
	startNewFile();
	print(	"digraph {\n"
		" labelloc=\"t\";"
		" label=\"time: ", m_prevTime.getTime(), "\";");
	for(const auto& item: m_cells){
		const CellData d = item.second;
		print("\n ", d);
	}
	for(auto& pIt1: m_conn){
		for(auto& pIt2: pIt1.second){
			if(pIt1.first < pIt2.first){
				auto revIt = m_conn[pIt2.first].find(pIt1.first);
				print("\n ", MovementData(pIt1.first, pIt2.first, pIt2.second, revIt->second));
				pIt2.second = 0;
				revIt->second = 0;
				LOG_DEBUG("Reset cell movement data from ", pIt1.first, " and ", pIt2.first, " to 0");
			}
		}
	}
	for(auto pIt1 = m_conn.begin(); pIt1 != m_conn.end(); ++pIt1){
		for(auto pIt2 = pIt1->second.begin(); pIt2 != pIt1->second.end(); ++pIt2){
			pIt2->second = 0;
		}
	}
	print("\n}");
	//clear all movement
}

void n_virus::VirusTracer::transitionTrace(t_timestamp time, std::vector<MovementData> movements, std::string mFrom,
        int senderValue, int senderProduction)

{
//	LOG_DEBUG("time: ", time, " from: ", mFrom, " to: ", mTo, " value: ", value, " @current time: ", time, " <> prevTime: ", m_prevTime);
	if (time.getTime() > m_prevTime.getTime()) {// || m_prevTime == t_timestamp(0, std::numeric_limits<t_timestamp::t_causal>::max())) {
		actualTrace(time);
		m_prevTime = time;
	}
	//save state of this model
	for(const MovementData& d: movements)
		m_conn[d.m_from][d.m_to] = d.m_amountLeft;
	LOG_DEBUG("Saving ", movements.size(), " movement updates @", m_prevTime.getTime());
	auto it = m_cells.find(mFrom);
	if(it == m_cells.end())
		m_cells.insert(std::make_pair(mFrom, CellData(mFrom, senderValue, senderProduction)));
	else
		it->second = CellData(mFrom, senderValue, senderProduction);

}
