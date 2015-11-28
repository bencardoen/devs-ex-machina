/*
 * Optimisticcore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen -- Tim Tuijn -- Stijn Manhaeve
 */

#include <thread>
#include "model/optimisticcore.h"
#include "tools/objectfactory.h"

using namespace n_model;
using namespace n_network;

Optimisticcore::~Optimisticcore()
{
        // Destructors are run on main(), our pool is live but we can't access it anymore.
        // Do not delete ptrs here, but we do want a trace should this happen.
#if (LOG_LEVEL!=0)
        for (t_msgptr ptr : m_sent_messages) {
                LOG_ERROR("MCORE:: ", this->getCoreID(), " HAVE ", ptr,
                        " in  m_sent_messages @ destruction. This will result in an std::bad_alloc exception.");
        }
#endif
        m_sent_messages.clear();
        // Another edge case, if we quit simulating before getting all messages from the network, we leak memory if 
        // any of these is an antimessage.

        if (m_network->havePendingMessages(this->getCoreID())) {
                LOG_ERROR("OCORE::", this->getCoreID(), " destructor detected messages in network for us, purging.");
                // Pull them in case another thread is waiting on network idle.
                std::vector<t_msgptr> msgs = m_network->getMessages(this->getCoreID());
        }
}

void Optimisticcore::initThread()
{
        for (size_t index = 0; index < m_indexed_models.size(); ++index) {
                const t_atomicmodelptr& model = m_indexed_models[index];
                model->setKeepOldStates(true);
                model->prepareSimulation();
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " preparing model ", model->getName(), " for simulation.");
        }
}

void Optimisticcore::shutDown()
{
        assert(!isLive() && "The core shouldn't be live when it is shut down!");
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " core shutting down, still need to remove ", m_sent_messages.size(),
                " messages");
        // @pre : not on main(), but on sim thread.
        for (auto& ptr : m_sent_messages) {
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " in core shutdown.");
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
        }
        for (auto& ptr : m_sent_antimessages) {
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " in core shutdown.");
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
        }
        for (const auto& ptr : m_indexed_models) {
                ptr->clearSentMessages();
                ptr->exitSimulation();
        }
        m_sent_messages.clear();
        m_sent_antimessages.clear();
}

Optimisticcore::Optimisticcore(const t_networkptr& net, std::size_t coreid, size_t cores)
        : Core(coreid, cores), m_network(net), m_color(MessageColor::WHITE), m_mcount_vector(cores), m_tred(
                t_timestamp::infinity()), m_tmin(0u), m_removeGVTMessages(false)
{
}

void Optimisticcore::clearProcessedMessages(std::vector<t_msgptr>& msgs)
{
#ifdef SAFETY_CHECKS
        if(msgs.size()==0)
        throw std::logic_error("Msgs not empty after processing ?");
#endif
        // In optimistic, delete only local-local messages after processing.
        for (t_msgptr& ptr : msgs) {
                ptr->setFlag(Status::PROCESSED);
                if (ptr->getSourceCore() == this->getCoreID() && ptr->getDestinationCore() == this->getCoreID()) {
                        m_stats.logStat(DELMSG);
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), "@", this->getTime(), " deleting ", ptr, " = ",
                                ptr->toString());
                        ptr->releaseMe();
                }
                else{
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), "@", this->getTime(), " Putting " , ptr->toString(), " in processed. &", ptr );
                        m_processed_messages.push_back(n_network::hazard_pointer(ptr));
                }
        }
        msgs.clear();
}

void Optimisticcore::sortMail(const std::vector<t_msgptr>& messages)
{
        for (const auto& message : messages) {
                message->setCausality(m_msgCurrentCount);
                m_msgCurrentCount = m_msgCurrentCount==m_msgEndCount? m_msgStartCount: (m_msgCurrentCount+1);

                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " sorting message ", message->toString());
                if (not this->isMessageLocal(message)) {
                        this->sendMessage(message);	// A noop for single core, multi core handles this.
                } else {
                        this->queueLocalMessage(message);
                }
        }
}

void Optimisticcore::sendMessage(t_msgptr msg)
{
        // We're locked on msglock. Don't change the ordering here.
        m_stats.logStat(MSGSENT);
        this->countMessage(msg);
        this->m_network->acceptMessage(msg);
        this->markMessageStored(msg);
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending message @", msg, " tostring: ", msg->toString());
}

void Optimisticcore::sendAntiMessage(const t_msgptr& msg)
{
        // An antimessage is still the same object, if you change the colour the 
        // counting (gvt) will be corrupted.
        m_stats.logStat(AMSGSENT);
        msg->setAntiMessage(true);
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending antimessage : ", msg->toString());
        m_sent_antimessages.push_back(msg);
        this->m_network->acceptMessage(msg);
}

void Optimisticcore::handleAntiMessage(const t_msgptr& msg)
{
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " entering handleAntiMessage with message ", msg);
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " handling antimessage ", msg->toString());
        // Storing the flag can speed up this process, but has to be done atomically because
        // we set KILL (which the sending thread reads).
        if (msg->flagIsSet(Status::PROCESSED)) {
                // Processed, so it is in m_processed, we can't touch it. Revert will clean it.
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " message already processed, should be in processed queue ", msg);
        } else if (msg->flagIsSet(Status::HEAPED)) {
                // is currently in the scheduler, mark as to erase
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " message only in heap, marking as TOERASE ", msg);
                m_received_messages->printScheduler();
                msg->setFlag(Status::ERASE);
        } else if (msg->flagIsSet(Status::DELETE)) {
                // we encountered this one before, just kill it.
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " original msg found, marking as KILL ", msg);
                msg->setFlag(Status::KILL);
        } else {
                // Not scheduled, not processed, so the first of a second pointer chasing.
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " first time we see this message, marking as DELETE ", msg);
                msg->setFlag(Status::DELETE);
        }
}

void Optimisticcore::markMessageStored(const t_msgptr& msg)
{
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " storing sent message ", msg, ": ", msg->toString());
#ifdef SAFETY_CHECKS
        if(msg->getSourceCore()!=this->getCoreID()) {
                LOG_FLUSH;
                throw std::logic_error("Storing msg not sent from this core.");
        }
#endif
        this->m_sent_messages.push_back(msg);
}

void Optimisticcore::countMessage(const t_msgptr& msg)
{
        /**
         * ALGORITHM 1.4 (or Fujimoto page 121 send algorithm)
         * @pre Msgcolor == this.color
         */
        std::lock_guard<std::mutex> colorlock(m_colorlock);
        msg->paint(m_color);
#ifdef SAFETY_CHECKS
        // With the new lock in this function, only memory corruption or sender changing color of this message could
        // trigger this check. Nonetheless, if it does we have a case of nasal demons, and want to know about it before they're seen.
        if(msg->getColor()!=m_color) {
                LOG_ERROR("Message color not equal to core color : ", msg->toString());
                LOG_FLUSH;
                throw std::logic_error("Msg color not equal to core color, race detected.");
        }
#endif
        if (m_color == MessageColor::WHITE) {           
                const size_t j = msg->getDestinationCore();
                std::lock_guard<std::mutex> lock(this->m_vlock);
                ++(this->m_mcount_vector.getVector()[j]);
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: incrementing count vector to : ", m_mcount_vector.getVector()[j]);
        } else {
                std::lock_guard<std::mutex> lock(this->m_tredlock);
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: tred pre ", m_tred);
                m_tred = std::min(m_tred, msg->getTimeStamp());
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: tred post : ", m_tred);
        }
}

void Optimisticcore::receiveMessage(t_msgptr msg)
{
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();
        bool msgtime_in_past = false;
        if (msgtime < this->getTime().getTime()) {
                msgtime_in_past = true;
        }

        m_stats.logStat(MSGRCVD);

        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message @", msg);

        if (msg->isAntiMessage()) {
                m_stats.logStat(AMSGRCVD);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got antimessage, not queueing.");
                this->handleAntiMessage(msg);
        } else {
                this->queuePendingMessage(msg);
                this->registerReceivedMessage(msg);
        }

        if (msgtime_in_past) {
                LOG_INFO("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", this->getTime());
                m_stats.logStat(REVERTS);
                this->revert(msgtime);
        }
}

void Optimisticcore::queuePendingMessage(t_msgptr msg)
{
        const MessageEntry entry(msg);
        if (!msg->flagIsSet(Status::HEAPED)) {
                this->m_received_messages->push_back(entry);
                msg->setFlag(Status::HEAPED);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " pushed message onto pending msgs: ", msg);
        } else {
                LOG_ERROR("\tCORE :: ", this->getCoreID(), " QPending messages already contains msg, overwriting ",
                        msg->toString());
                throw std::logic_error("Pending msgs integrity failure.");
        }
}

void Optimisticcore::registerReceivedMessage(const t_msgptr& msg)
{
        // ALGORITHM 1.5 (or Fujimoto page 121 receive algorithm)
        // msg is not an antimessage here.
        if (msg->getColor() == MessageColor::WHITE) {
                std::lock_guard<std::mutex> lock(this->m_vlock);
                --this->m_mcount_vector.getVector()[this->getCoreID()];
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: decrementing count vector to : ",
                        m_mcount_vector.getVector()[this->getCoreID()]);
        }
}

void Optimisticcore::gcCollect()
{
        auto senditer = m_sent_messages.begin();
        for (; senditer != m_sent_messages.end(); ++senditer) {
                if ((*senditer)->getTimeStamp().getTime() >= this->getGVT().getTime()) {    // time value only ?
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " found msg >= gvt, stopping collect ", (*senditer)->toString());
                        break;
                }
                
                t_msgptr& ptr = *senditer;
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " gcollecting ", ptr);
                LOG_FLUSH;
                // If gvt is correct, messages has to be processed, and heap is not set.
#ifdef SAFETY_CHECKS
                if(!ptr->flagIsSet(Status::PROCESSED) || ptr->flagIsSet(Status::HEAPED)){
                        LOG_ERROR("GVT integrity failure :: id= ", this->getCoreID(), " for msg ", ptr, " not processed but gvt is past tstamp?");
                        // todo exception ?
                        break;
                }
#endif        
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " = ", ptr->toString());
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
#ifdef SAFETY_CHECKS
                ptr = nullptr;
#endif
        }

        LOG_DEBUG("MCORE:: ", this->getCoreID(), " gccollecting antimessages ");
        for (auto aiter = m_sent_antimessages.begin(); aiter != m_sent_antimessages.end();) {
                t_msgptr ptr = *aiter;
                if (ptr->flagIsSet(Status::KILL)) {
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting antimessage ", ptr, " = ", ptr->toString());
                        ptr->releaseMe();
                        m_stats.logStat(DELMSG);
                        aiter = m_sent_antimessages.erase(aiter);
                } else {
                        ++aiter;
                }
        }

        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " found ",
                distance(m_sent_messages.begin(), senditer), " sent messages to erase.");

        m_sent_messages.erase(m_sent_messages.begin(), senditer);
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " sent messages now contains :: ",
                m_sent_messages.size());
        
        auto piter = m_processed_messages.begin();
        for(;piter != m_processed_messages.end();++piter){
                hazard_pointer hp = *piter;             // Don't access the pointer until we know it is safe.
                if(hp.m_msgtime >= this->getGVT().getTime()){
                        break;
                }
        }
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " erasing ",
                std::distance(m_processed_messages.begin(), piter), " processed messages. ");
        m_processed_messages.erase(m_processed_messages.begin(), piter);
        
        m_removeGVTMessages = false;

        LOG_DEBUG("MCORE:: ", this->getCoreID(), " calling setGVT on all models.");

        n_network::t_timestamp newgvt = getGVT();
        for (const auto& model : this->m_indexed_models)
                model->setGVT(newgvt);
}

void Optimisticcore::runSmallStep()
{
        this->lockSimulatorStep();

        if (m_removeGVTMessages) {
                gcCollect();
        }

        m_stats.logStat(TURNS);

  
        this->getMessages();

        if (!this->isLive()) {
            LOG_DEBUG("\tCORE :: ", this->getCoreID(),
                    " skipping small Step, we're idle and got no messages.");
            this->unlockSimulatorStep();
            return;
        }

        this->getImminent(m_imminents);

        this->collectOutput(m_imminents);

        this->getPendingMail();

        this->transition();

        this->rescheduleImminent();

        this->syncTime();               
        m_imminents.clear();
        m_externs.clear();

        this->checkTerminationFunction();

        this->unlockSimulatorStep();
}

void Optimisticcore::getMessages()
{
        bool wasLive = isLive();
        this->setLive(true);
        if (!wasLive) {
                LOG_INFO("MCORE :: ", this->getCoreID(), " switching to live before we check for messages");
        }
        if (this->m_network->havePendingMessages(this->getCoreID())) {
                std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
                LOG_INFO("CCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
                this->sortIncoming(messages);
        } else {
                if (!wasLive) {
                        setLive(false);
                        LOG_INFO("MCORE :: ", this->getCoreID(),
                                " switching back to not live. No messages from network and we weren't live to begin with.");
                }
        }
}

void Optimisticcore::sortIncoming(const std::vector<t_msgptr>& messages)
{
        for (auto i = messages.begin(); i != messages.end(); i++) {
                const auto & message = *i;
#ifdef SAFETY_CHECKS
                validateUUID(uuid(message->getDestinationCore(), message->getDestinationModel()));
#endif
                this->receiveMessage(message);
        }
}

void Optimisticcore::waitUntilOK(const t_controlmsg& msg, std::atomic<bool>& rungvt)
{
        // We don't have to get the count from the message each time,
        // because the control message doesn't change, it stays in this core
        // The V vector of this core can change because ordinary message are
        // still being received by another thread in this core, this is why
        // we lock the V vector
        const int msgcount = msg->getCountVector()[this->getCoreID()];
        while (true) {

                if (rungvt == false) {
                        LOG_INFO("MCORE :: ", this->getCoreID(),
                                " rungvt set to false by a Core thread, stopping GVT.");
                        return;
                }
                int v_value = 0;
                {
                        std::lock_guard<std::mutex> lock(m_vlock);
                        v_value = this->m_mcount_vector.getVector()[this->getCoreID()];
                }
                if (v_value + msgcount <= 0) {
                        LOG_DEBUG("MCORE :: ", this->getCoreID(), " rungvt : V + C <=0; v= ", v_value, " C=", msgcount);
                        break;
                }
                // We can't use a cvar here, if sim ends before we wake we hang the entire simulation.
                {
                        std::chrono::milliseconds ms { 1 };
                        std::this_thread::sleep_for(ms);
                }
        }
}

void Optimisticcore::receiveControl(const t_controlmsg& msg, int round, std::atomic<bool>& rungvt)
{
// ALGORITHM 1.7 (more or less) (or Fujimoto page 121)
// Also see snapshot_gvt.pdf
        // Race check : id = const, read only.
        if (rungvt == false) {
                LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a thread, stopping GVT.");
                return;
        }
        if (this->getCoreID() == 0 && round == 0) {
                startGVTProcess(msg, round, rungvt);
        } else if (this->getCoreID() == 0) {            // Round 1,2,3...
                finalizeGVTRound(msg, round, rungvt);
        } else {
                receiveControlWorker(msg, round, rungvt);
        }
}

void Optimisticcore::finalizeGVTRound(const t_controlmsg& msg, int round, std::atomic<bool>& rungvt)
{
        LOG_DEBUG("MCORE :: ", this->getCoreID(),
                " GVT : process init received control message, waiting for pending messages.");
        this->waitUntilOK(msg, rungvt);
        LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT : All messages received, checking vectors. ");
        if (msg->countIsZero()) {  /// Case found GVT
                LOG_INFO("MCORE :: ", this->getCoreID(), " found GVT!");
                t_timestamp GVT_approx = std::min(msg->getTmin(), msg->getTred());
                LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT approximation = min( ", msg->getTmin(), ",",
                        msg->getTred(), ")");
                msg->setGvtFound(true);
                msg->setGvt(GVT_approx);
        } else {                /// C vector is != 0
                if (round == 1) {
                        LOG_DEBUG("MCORE :: ", this->getCoreID(),
                                " process init received control message, starting 2nd round");
                        msg->setTmin(this->getTMin());
                        msg->setTred(std::min(msg->getTred(), this->getTred()));
                        LOG_DEBUG("MCORE :: ", this->getCoreID(), " Starting 2nd round with tmin ", msg->getTmin(),
                                " tred ", msg->getTred(), ")");
                        t_count& count = msg->getCountVector();
                        std::lock_guard<std::mutex> lock(this->m_vlock);
                        for (size_t i = 0; i < count.size(); ++i) {
                                count[i] += this->m_mcount_vector.getVector()[i];
                                this->m_mcount_vector.getVector()[i] = 0;
                        }
                } else {
                        LOG_DEBUG("MCORE :: ", this->getCoreID(),
                                " 2nd round , P0 still has non-zero count vectors, quitting algorithm");
                }
                /// There is no cleanup to be done, startGVT initializes core/msg.
        }
}

void Optimisticcore::receiveControlWorker(const t_controlmsg& msg, int /*round*/, std::atomic<bool>& rungvt)
{
        // ALGORITHM 1.6 (or Fujimoto page 121 control message receive algorithm)
        if (this->getColor() == MessageColor::WHITE) {
                this->setTred(t_timestamp::infinity());
                this->setColor(MessageColor::RED);
        }
        // We wait until we have received all messages
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " process received control message");
        waitUntilOK(msg, rungvt);

        // Equivalent to sending message, controlmessage is passed to next core.
        t_timestamp msg_tmin = msg->getTmin();
        t_timestamp msg_tred = msg->getTred();
        msg->setTmin(std::min(msg_tmin, this->getTMin()));
        msg->setTred(std::min(msg_tred, this->getTred()));
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " Updating tmin to ", msg->getTmin(), " tred = ",
                msg->getTred());
        t_count& Count = msg->getCountVector();
        std::lock_guard<std::mutex> lock(this->m_vlock);
        for (size_t i = 0; i < Count.size(); ++i) {
                Count[i] += this->m_mcount_vector.getVector()[i];
                this->m_mcount_vector.getVector()[i] = 0;
        }

        // Send message to next process in ring
        return;
}

void Optimisticcore::startGVTProcess(const t_controlmsg& msg, int /*round*/, std::atomic<bool>& rungvt)
{
        if (rungvt == false) {
                LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a thread, stopping GVT.");
                return;
        }
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(),
                " GVT received first control message, starting first round");
        this->setColor(MessageColor::RED);
        setTred(t_timestamp::infinity());
        msg->setTmin(this->getTMin());
        msg->setTred(t_timestamp::infinity());
        LOG_INFO("MCORE :: ", this->getCoreID(), " Starting GVT Calculating with tred value of ", msg->getTred(),
                " tmin = ", msg->getTmin());
        t_count& count = msg->getCountVector();
        // We want to make sure our count vector starts with 0
        std::fill(count.begin(), count.end(), 0);
        // Lock because we will change our V vector
        std::lock_guard<std::mutex> lock(this->m_vlock);
        for (size_t i = 0; i < count.size(); i++) {
                count[i] += this->m_mcount_vector.getVector()[i];
                this->m_mcount_vector.getVector()[i] = 0;
        }
        // Send message to next process in ring
        return;
}

void Optimisticcore::setGVT(const t_timestamp& candidate)
{
        this->lockSimulatorStep();              
        t_timestamp newgvt = t_timestamp(candidate.getTime(), 0);
#ifdef SAFETY_CHECKS
        if (newgvt.getTime() < this->getGVT().getTime() || isInfinity(newgvt) ) {
                LOG_WARNING("Core:: ", this->getCoreID(), " cowardly refusing to set gvt to ", newgvt, " vs current : ",
                        this->getGVT());
                LOG_FLUSH;
                this->unlockSimulatorStep();
                throw std::logic_error("Invalid GVT found");
        }
#endif

        Core::setGVT(newgvt);
        m_removeGVTMessages = true;

        // Reset state (note V-vector is reset by Mattern code.

        this->setColor(MessageColor::WHITE);
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(),
                " painted core back to white, for next gvt calculation");

        this->unlockSimulatorStep();
}

void n_model::Optimisticcore::lockSimulatorStep()
{
        LOG_DEBUG("MCORE :: ", this->getCoreID(), " trying to lock simulator core");
        this->m_locallock.lock();       
        LOG_DEBUG("MCORE :: ", this->getCoreID(), "simulator core locked");

}

void n_model::Optimisticcore::unlockSimulatorStep()
{
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " trying to unlock simulator core.");
        this->m_locallock.unlock();
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " simulator core unlocked.");
}

void n_model::Optimisticcore::revert(const t_timestamp& rtime)
{
        const t_timestamp::t_time totime = rtime.getTime();
        const t_timestamp::t_time gtime = this->getGVT().getTime();
        assert(totime >= gtime);
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting from ", this->getTime(), " to ", totime);
        if (!this->isLive()) {
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " Core going from idle to active ");
                this->setLive(true);
                this->setTerminatedByFunctor(false);
        }

        LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting on a total of ", m_sent_messages.size(), " sent messages");
        while (!m_sent_messages.empty()) {		// For each message > totime, send antimessage
                t_msgptr msg = m_sent_messages.back();
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting message ", msg, " ", msg->toString());
                if (msg->getTimeStamp().getTime() >= totime) {
                        m_sent_messages.pop_back();
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(),
                                " revert : sent message > time , antimessagging. \n ", msg->toString());
                        this->sendAntiMessage(msg);
                } else {
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " message time < current time ", msg);
                        break;
                }
        }
        
        while(m_processed_messages.size()){
                // DO NOT access the pointer itself
                hazard_pointer msg = m_processed_messages.back();
                LOG_DEBUG("Revert :: handling processed msg :: ", msg.m_ptr, " ~ ");
                t_timestamp::t_time msgtime = msg.m_msgtime;
                if(msgtime < totime)
                        break;
                // From here the pointer is safe (if GVT is not failing.)
#ifdef SAFETY_CHECKS
                if(!msg.m_ptr->flagIsSet(Status::PROCESSED)){
                        LOG_DEBUG("P not set on msg :: ", msg.m_ptr, " ");
                        throw std::logic_error("P not set on a PROCESSED msg.");
                }
#endif
                if(!msg.m_ptr->flagIsSet(Status::ANTI)){
                        msg.m_ptr->setFlag(Status::HEAPED);
                        msg.m_ptr->setFlag(Status::PROCESSED, false);
                        m_received_messages->push_back(msg.m_ptr);
                }
                m_processed_messages.pop_back();
        }
        
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " Done with reverting messages.");

        this->setTime(rtime);
        this->rescheduleAllRevert(rtime);		// Make sure the scheduler is reloaded with fresh/stale models
        this->revertTracerUntil(rtime); 	// Finally, revert trace output
}

bool n_model::Optimisticcore::existTransientMessage()
{
        bool b = this->m_network->empty();
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " network is empty = ", b);
        return !b;
}

void n_model::Optimisticcore::setColor(MessageColor mc)
{
        std::lock_guard<std::mutex> lock(m_colorlock);
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " setting color from ", (m_color==WHITE?"white":"red"), " to ", (mc==WHITE?"white":"red"));
        this->m_color = mc;
}

MessageColor n_model::Optimisticcore::getColor()
{
        std::lock_guard<std::mutex> lock(m_colorlock);
        return m_color;
}

/**
 * Locking strategy explained :
 *      get/set time has a very high call frequency
 *      sim thread : write/read on time
 *      gvt thread : read on time.
 *      So if we share tmin between them (which is what gvt uses time for), and update (locked)
 *      that value sim t updates time, we avoid the unnecessary lock on reading our own writes.
 */
void n_model::Optimisticcore::setTime(const t_timestamp& newtime)
{
        this->setTMin(newtime);
        Core::setTime(newtime);
}

t_timestamp n_model::Optimisticcore::getTred()
{
        std::lock_guard<std::mutex> lock(m_tredlock);
        return this->m_tred;
}

void n_model::Optimisticcore::setTred(t_timestamp val)
{
        std::lock_guard<std::mutex> lock(m_tredlock);
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " setting tRed from ", m_tred, " to ", val);
        this->m_tred = val;
}


t_timestamp 
n_model::Optimisticcore::getFirstMessageTime()
{
        // Todo could test HEAPED.
        t_timestamp mintime = t_timestamp::infinity();
        while (not this->m_received_messages->empty()) {
                const MessageEntry& msg = m_received_messages->top();
                if(msg.getMessage()->flagIsSet(Status::ERASE)){
                        t_msgptr msgptr = msg.getMessage();
                        m_received_messages->pop();
                        msgptr->setFlag(Status::HEAPED, false);
                        msgptr->setFlag(Status::KILL);
                        continue;
                }
                mintime = this->m_received_messages->top().getMessage()->getTimeStamp();
                break;
        }
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " first message time == ", mintime);
        return mintime;
}

