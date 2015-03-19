/*
 * core.h
 *      Author: Ben Cardoen
 */
#include <timestamp.h>
#include <network.h>
//#include <model.h>	// TODO uncomment if models are ready.
#include "scheduler.h"
#include "schedulerfactory.h"
#include "modelentry.h"

#ifndef SRC_MODEL_CORE_H_
#define SRC_MODEL_CORE_H_

namespace n_model {

using n_network::t_networkptr;
using n_network::t_msgptr;
using n_network::t_timestamp;

// Stub to allow testing without breaking interface with Model
// TODO replace with Model
struct modelstub
{
	std::string name;
	modelstub(std::string s)
		: name(s)
	{
		;
	}
	std::string getName()
	{
		return name;
	}
};
typedef std::shared_ptr<modelstub> t_modelptr;	// TODO remove stubbed typedef if models are live.

typedef std::shared_ptr<n_tools::Scheduler<ModelEntry>> t_scheduler;

/**
 * A Core is a node in a parallel devs simulator. It manages (multiple) models and drives their transitions.
 */
class Core
{
private:
	t_networkptr m_network;
	t_timestamp m_time;
	std::size_t m_coreid;
	std::unordered_map<std::string, t_modelptr> m_models;
	bool m_live;
	t_scheduler m_scheduler;

public:
	/**
	 * Default single core implementation.
	 * @attention : network = nullptr !!
	 */
	Core();

	/**
	 * Multicore implementation.
	 * @pre netlink has at least id queues.
	 */
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
	 * @TODO protected
	 */
	virtual
	void receiveMessages(std::vector<t_msgptr>&);

	/**
	 * Send all collected messages to network.
	 * @TODO protected
	 */
	virtual
	void sendMessages();

	/**
	 * Indicates if Core is running, or halted.
	 */
	bool isLive() const
	{
		return m_live;
	}

	/**
	 * Start/Stop core.
	 */
	void setLive(bool live)
	{
		m_live = live;
	}

	std::size_t getCoreID() const
	{
		return m_coreid;
	}

	// 3 (condensed) stages : output from all models, distr messages, transition, repeat
	/**
	 * Iterate over models, collecting output and forward it to tracing.
	 */
	virtual void
	collectOutput();

	/**
	 * Request from all models any triggered messages, deliver outbound messages to network,
	 * get remote messages and hand them off to models.
	 */
	virtual void
	routeMessages();

	/**
	 * Depending on whether a model may transition (imminent), and/or has received messages, transition.
	 */
	virtual void
	transition();

	/**
	 * Schedule model.name @ time t.
	 * @TODO make private
	 */
	void
	scheduleModel(std::string name, t_timestamp t);
};

typedef std::shared_ptr<Core> t_coreptr;

// Prototype running function (to pass to thread).
void runCore(t_coreptr);

}

#endif /* SRC_MODEL_CORE_H_ */
