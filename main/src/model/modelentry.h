/*
 * modelentry.h
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_MODELENTRY_H_
#define SRC_MODEL_MODELENTRY_H_

namespace n_model {

using n_network::t_timestamp;

/**
 * Entry for a Model in a scheduler.
 * Keeps modelname and imminent time for a Model, without having to store the entire model.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class ModelEntry
{
	std::string m_name;
	t_timestamp m_scheduled_at;
public:
	std::string getName() const
	{
		return m_name;
	}
	t_timestamp getTime() const
	{
		return m_scheduled_at;
	}

	ModelEntry() = default;
	~ModelEntry() = default;
	ModelEntry(const ModelEntry&) = default;
	ModelEntry(ModelEntry&&) = default;
	ModelEntry& operator=(const ModelEntry&) = default;
	ModelEntry& operator=(ModelEntry&&) = default;

	ModelEntry(std::string name, t_timestamp time)
		: m_name(name), m_scheduled_at(time)
	{
		;
	}

	friend
	bool operator<(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		if (lhs.m_scheduled_at == rhs.m_scheduled_at)
			return lhs.m_name > rhs.m_name;
		return lhs.m_scheduled_at > rhs.m_scheduled_at;
	}

	friend
	bool operator>(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (rhs < lhs);
	}

	friend
	bool operator>=(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (!(lhs < rhs));
	}

	friend
	bool operator==(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (lhs.m_name == rhs.m_name /*&& lhs.m_scheduled_at == rhs.m_scheduled_at*/); // uncomment to allow multiple entries per model
	}

	friend
	std::ostream& operator<<(std::ostream& os, const ModelEntry& rhs){
		return (os<<rhs.getName() << " scheduled at " << rhs.m_scheduled_at);
	}
};

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

#endif /* SRC_MODEL_MODELENTRY_H_ */
