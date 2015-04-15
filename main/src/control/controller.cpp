/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"
#include <deque>

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
        std::shared_ptr<Allocator> alloc, std::shared_ptr<LocationTable> locTab, n_tracers::t_tracersetptr tracers)
	: m_isClassicDEVS(true), m_isDSDEVS(false), m_hasMainModel(false), m_isSimulating(false), m_name(name),
	  m_checkTermTime(false), m_checkTermCond(false), m_cores(cores), m_locTab(locTab), m_allocator(alloc),
	  m_tracers(tracers), m_dsPhase(false)
{
	m_root = n_tools::createObject<n_model::RootModel>();
}

Controller::~Controller()
{
}

void Controller::addModel(t_atomicmodelptr& atomic)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	size_t coreID = m_allocator->allocate(atomic);
	addModel(atomic, coreID);
	m_hasMainModel = true;
}

void Controller::doDirectConnect()
{
	if(m_coupledOrigin){
		m_root->directConnect(m_coupledOrigin);
	}
}

void Controller::doDSDevs(std::vector<n_model::t_atomicmodelptr>& imminent)
{
	m_dsPhase = true;
	// loop over all atomic models
	std::deque<t_modelptr> models;
	for (t_atomicmodelptr& model : imminent) {
		if (model->modelTransition(&m_sharedState)) {
			// keep a reference to the parent of this model
			if(model->getParent().expired())
				continue;
			t_modelptr parent = model->getParent().lock();
			if (parent)
				models.push_back(parent);
		}
	}
	// continue looping until we’re at the top of the tree
	while (models.size()) {
		t_modelptr top = models.front();
		models.pop_front();
		if (top->modelTransition(&m_sharedState)) {
			// do the DS transition
			if (!top->modelTransition(&m_sharedState))
				continue; // no need to continue
			// if nothing happened .
			// keep a reference to the parent of this model
			if(top->getParent().expired())
				continue;
			t_modelptr parent = top->getParent().lock();
			if (parent)
				models.push_back(parent);
		}
	}
	// perform direct connect
	// if nothing structurally changed , nothing will happen .
	doDirectConnect();
	m_dsPhase = false;
}

void Controller::dsAddConnection(const n_model::t_portptr&, const n_model::t_portptr&, const t_zfunc&)
{
	assert(isInDSPhase() && "Controller::dsAddConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemoveConnection(const n_model::t_portptr&, const n_model::t_portptr&)
{
	assert(isInDSPhase() && "Controller::dsRemoveConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemovePort(n_model::t_portptr&)
{
	assert(isInDSPhase() && "Controller::dsRemovePort called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsScheduleModel(const n_model::t_modelptr& model)
{
	assert(isInDSPhase() && "Controller::dsScheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();
	//recursively add submodels, if necessary
	t_coupledmodelptr coupled = std::dynamic_pointer_cast<CoupledModel>(model);
	if(coupled){
		//it is a coupled model, remove all its children
		std::vector<t_modelptr> comp = coupled->getComponents();
		for(t_modelptr& sub:comp)
			dsScheduleModel(sub);
		return;
	}
	t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel>(model);
	if(atomic){
		//it is an atomic model. Just remove this one from the core and the root devs
		m_cores.begin()->second->addModel(atomic);
		//no need to remove the model from the root devs. We have to redo direct connect anyway
		return;
	}
}

void Controller::dsUnscheduleModel(n_model::t_modelptr& model)
{
	assert(isInDSPhase() && "Controller::dsUnscheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();
	//TODO move most of this stuff to the model side of the implementation
	//remove all ports
	std::map<std::string, t_portptr> ports = model->getIPorts();
	for(std::map<std::string, t_portptr>::value_type& port: ports)
		model->removePort(port.second);
	ports = model->getOPorts();
	for(std::map<std::string, t_portptr>::value_type& port: ports)
		model->removePort(port.second);
	t_coupledmodelptr coupled = std::dynamic_pointer_cast<CoupledModel>(model);
	if(coupled){
		//it is a coupled model, remove all its children
		std::vector<t_modelptr> comp = coupled->getComponents();
		for(t_modelptr& sub:comp)
			coupled->removeSubModel(sub);
		return;
	}
	t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel>(model);
	if(atomic){
		//it is an atomic model. Just remove this one from the core and the root devs
		m_cores.begin()->second->removeModel(atomic->getName());
		//no need to remove the model from the root devs. We have to redo direct connect anyway
		return;
	}
	throw std::logic_error("Controller::dsUnscheduleModel requested to unschedule a model that is neither a coupled model nor an atomic model.");
}

void Controller::dsUndoDirectConnect()
{
	assert(isInDSPhase() && "Controller::dsUndoDirectConnect called while not in the DS phase.");
	m_root->undoDirectConnect();
}

bool Controller::isInDSPhase() const
{
	return m_dsPhase;
}

void Controller::addModel(t_atomicmodelptr& atomic, std::size_t coreID)
{
	m_cores[coreID]->addModel(atomic);
	m_locTab->registerModel(atomic, coreID);
}

void Controller::addModel(t_coupledmodelptr& coupled)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	assert(coupled != nullptr && "Cannot add nullptr as origin coupled model.");

	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	throw std::logic_error("Controller : simDSDEVS not implemented");
	m_coupledOrigin = coupled;
	const std::vector<t_atomicmodelptr>& atomics = m_root->directConnect(coupled);
	for( auto at : atomics ) {
		addModel(at);
	}
	m_hasMainModel = true;
}

void Controller::simulate()
{
//	if (!m_tracers->isInitialized()) {
//		// TODO ERROR
//	}
	assert(m_isSimulating == false && "Can't start a simulation while already simulating.");

	if (!m_hasMainModel) {
		// nothing to do, so don't even start
		LOG_WARNING("CONTROLLER: Trying to run simulation without any models!");
		return;
	}

	m_isSimulating = true;

	if (!m_isClassicDEVS && m_checkpointInterval.getTime() > 0) { // checkpointing is active
		startGVTThread();
	}

	// configure all cores
	for (auto core : m_cores) {
		core.second->setTracers(m_tracers);
		core.second->init();
		if (m_checkTermTime) core.second->setTerminationTime(m_terminationTime);
		if (m_checkTermCond) core.second->setTerminationFunction(m_terminationCondition);
		core.second->setLive(true);
	}

	// run simulation
	if (m_isDSDEVS) {
		simDSDEVS();
	} else {
		simDEVS();
	}

	m_isSimulating = false;
}

void Controller::simDEVS()
{
	uint i = 0;
	while (check()) { // As long any cores are active
		++i;
		LOG_INFO("CONTROLLER: Commencing simulation loop #", i, "...");
		for (auto core : m_cores) {
			if (core.second->isLive()) {
				LOG_INFO("CONTROLLER: Core ", core.second->getCoreID(), " starting small step.");
				core.second->runSmallStep();
			} else LOG_INFO("CONTROLLER: Shhh, core ", core.second->getCoreID(), " is resting now.");
		}
	}
	LOG_INFO("CONTROLLER: All cores terminated, simulation finished.");
}

void Controller::simDSDEVS()
{
	t_coreptr& core = m_cores.begin()->second;
	std::vector<n_model::t_atomicmodelptr> imminent;
	std::size_t i = 0;
	while(!core->terminated()) {
		++i;
		imminent.clear();
		LOG_INFO("CONTROLLER: Commencing DSDEVS simulation loop #", i, "...");
		if(core->isLive()){
			LOG_INFO("CONTROLLER: DSDEVS Core ", core->getCoreID(), " starting small step.");
			core->runSmallStep();
			core->getLastImminents(imminent);
			doDSDevs(imminent);
		}
	}
}

void Controller::setClassicDEVS(bool classicDEVS)
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_isClassicDEVS = classicDEVS;
	if (!classicDEVS)
		m_isDSDEVS = false;
}

void Controller::setDSDEVS(bool dsdevs)
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	if (dsdevs)
		m_isClassicDEVS = true;
	m_isDSDEVS = dsdevs;
}

void Controller::setTerminationTime(t_timestamp time)
{
	assert(m_isSimulating == false && "Cannot change termination time during simulation");
	m_checkTermTime = true;
	m_terminationTime = time;
}

void Controller::setTerminationCondition(t_terminationfunctor termination_condition)
{
	assert(m_isSimulating == false && "Cannot change termination condition during simulation");
	m_checkTermCond = true;
	m_terminationCondition = termination_condition;
}

void Controller::setCheckpointInterval(t_timestamp interv)
{
	m_checkpointInterval = interv;
}

void Controller::startGVTThread()
{
	throw std::logic_error("Controller : startGVTThread not implemented");
}

void Controller::waitFinish(size_t)
{
	throw std::logic_error("Controller : waitFinish not implemented");
}

bool Controller::check()
{
	for (auto core : m_cores) {
		if (!core.second->terminated())
			return true;
	}
	return false;
}

void Controller::emptyAllCores()
{
	for (auto core : m_cores) {
		core.second->clearModels();
	}
	m_root = n_tools::createObject<n_model::RootModel>(); // reset root
}

} /* namespace n_control */
