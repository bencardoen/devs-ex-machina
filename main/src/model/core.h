/*
 * core.h
 *      Author: Ben Cardoen
 */
#include <timestamp.h>
#include <network.h>
#include <model.h>
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
			return lhs.m_name < rhs.m_name;
		return lhs.m_scheduled_at < rhs.m_scheduled_at;
	}

	friend
	bool operator==(const ModelEntry& lhs, const ModelEntry& rhs){
		return (lhs.m_name==rhs.m_name && lhs.m_scheduled_at==rhs.m_scheduled_at);// uncomment to allow multiple entries per model
	}
};

/**
 * A Core is a node in a parallell devs simulator. It manages models and drives their transitions.
 */
class Core{
private:
	t_networkptr	m_network;
	t_timestamp 	m_time;
	std::unordered_map<std::string, t_modelptr> m_models;
	// Scheduler

public:
	/**
	 * Default single core implementation.
	 */
	Core();
	virtual ~Core() = default;

	void save(const std::string& fname);
	void load(const std::string& fname);

	void revert(t_timestamp totime);

	void addModel(t_modelptr model);

	void removeModel(std::string mname);

	t_modelptr
	getModel(std::string name);

	virtual
	void receiveMessages();

	virtual
	void sendMessages();
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
		std::cout << "Hash function for "<< item.getName()<<std::endl;
		return hash<std::string>()(item.getName());
	}
};
}



#endif /* SRC_MODEL_CORE_H_ */
