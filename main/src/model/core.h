/*
 * core.h
 *      Author: Ben Cardoen
 */
#include <timestamp.h>
#include <network.h>
//#include <model.h>
#include <scheduler.h>
#include <schedulerfactory.h>

#ifndef SRC_MODEL_CORE_H_
#define SRC_MODEL_CORE_H_

namespace n_model{

using n_network::t_networkptr;
using n_network::t_msgptr;
using n_network::t_timestamp;

/**
 * Entry for a Model in a scheduler.
 * Keeps modelname and imminent time for a Model, without having to store the entire model.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class ModelEntry{
	std::string m_name;
	t_timestamp m_scheduled_at;
public:
	std::string getName()const{return m_name;}
	t_timestamp getTime()const{return m_scheduled_at;}

	ModelEntry() = default;
	~ModelEntry() = default;
	ModelEntry(const ModelEntry&) = default;
	ModelEntry( ModelEntry&&) = default;
	ModelEntry& operator=(const ModelEntry&)= default;
	ModelEntry& operator=(ModelEntry&&)= default;

	ModelEntry(std::string name , t_timestamp time):m_name(name), m_scheduled_at(time){;}

	friend
	bool operator<(const ModelEntry& lhs, const ModelEntry& rhs){
		if(lhs.m_scheduled_at == rhs.m_scheduled_at)
			return lhs.m_name > rhs.m_name;
		return lhs.m_scheduled_at > rhs.m_scheduled_at;
	}

	friend
	bool operator>(const ModelEntry& lhs, const ModelEntry& rhs){
		return (rhs < lhs);
	}

	friend
	bool operator>=(const ModelEntry& lhs, const ModelEntry& rhs){
		return (! (lhs<rhs));
	}

	friend
	bool operator==(const ModelEntry& lhs, const ModelEntry& rhs){
		return (lhs.m_name==rhs.m_name && lhs.m_scheduled_at==rhs.m_scheduled_at);// uncomment to allow multiple entries per model
	}
};

struct modelstub{
std::string name;
modelstub(std::string s):name(s){;}
std::string getName(){return name;}
};

typedef std::shared_ptr<modelstub> t_modelptr;	// TODO remove stubbed typedef if models are live.

typedef std::shared_ptr<n_tools::Scheduler<ModelEntry>> 	t_scheduler;

/**
 * A Core is a node in a parallel devs simulator. It manages models and drives their transitions.
 */
class Core{
private:
	t_networkptr	m_network;
	t_timestamp 	m_time;
	std::size_t	m_coreid;
	std::unordered_map<std::string, t_modelptr> m_models;
	bool		m_live;
	t_scheduler	m_scheduler;

public:
	// 3 (condensed) stages : output from all models, distr messages, transition, repeat
	virtual void
	collectOutput();

	virtual void
	routeMessages();

	virtual void
	transition();

	void
	scheduleModel(std::string name, t_timestamp t);

public:
	/**
	 * Default single core implementation.
	 */
	Core();

	Core(std::size_t id, const t_networkptr& netlink);
	virtual ~Core() = default;

	/**
	 * Serialize this core to file fname.
	 */
	void save(const std::string& fname);

	/**
	 * Load this core from file fname;
	 */
	void load(const std::string& fname);

	/**
	 * In optimistic simulation, revert models to earlier stage defined by totime.
	 */
	void revert(t_timestamp totime);

	/**
	 * Add model to this core.
	 */
	void addModel(t_modelptr model);

	/**
	 * Retrieve model with name from core
	 * @attention does not change anything in scheduled order.
	 */
	t_modelptr
	getModel(std::string name);

	/**
	 * Pull all messages from network for processing.
	 */
	virtual
	void receiveMessages(std::vector<t_msgptr>&);

	/**
	 * Send all collected messages to network.
	 */
	virtual
	void sendMessages();

	/**
	 * Indicates if Core is running, or halted.
	 */
	bool isLive() const{return m_live;}

	/**
	 * Start/Stop core.
	 */
	void setLive(bool live){m_live = live;}
};

typedef std::shared_ptr<Core> t_coreptr;

// Prototype running function (to pass to thread).
void runCore(t_coreptr);

}

namespace std {
template<>
struct hash<n_model::ModelEntry>
{
	size_t operator()(const n_model::ModelEntry& item) const
	{
		//std::cout << "Hash function for "<< item.getName()<<std::endl;
		return hash<std::string>()(item.getName());
	}
};
}



#endif /* SRC_MODEL_CORE_H_ */
