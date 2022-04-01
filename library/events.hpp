#pragma once
#include <functional>
#include <list>

template<class ...Arguments >
class EventProvider {
	std::list<std::function<bool(Arguments...)>> heigh_priorihty;
	std::list<std::function<bool(Arguments...)>> upper_avg_priorihty;
	std::list<std::function<bool(Arguments...)>> avg_priorihty;
	std::list<std::function<bool(Arguments...)>> lower_avg_priorihty;
	std::list<std::function<bool(Arguments...)>> low_priorihty;
	bool can_canceled;
public:
	enum class Priorithy {
		heigh,
		upper_avg,
		avg,
		lower_avg,
		low
	};

	EventProvider(bool cancelable = true) : can_canceled(cancelable) {}
	void operator+=(const std::function<bool(Arguments...)>& func) {
		avg_priorihty.push_back(func);
	}
	void join(const std::function<bool(Arguments...)>& func, Priorithy priorithy = Priorithy::avg) {
		switch (priorithy)
		{
		case Priorithy::heigh:
			heigh_priorihty.push_back(func);
			break;
		case Priorithy::upper_avg:
			upper_avg_priorihty.push_back(func);
			break;
		case Priorithy::avg:
			avg_priorihty.push_back(func);
			break;
		case Priorithy::lower_avg:
			lower_avg_priorihty.push_back(func);
			break;
		case Priorithy::low:
			low_priorihty.push_back(func);
			break;
		default:
			break;
		}
	}

	bool operator()(Arguments ... args) {
		for (auto& tmp : heigh_priorihty)
			if (tmp(args...) && can_canceled)
				return true;
		for (auto& tmp : upper_avg_priorihty)
			if (tmp(args...) && can_canceled)
				return true;
		for (auto& tmp : avg_priorihty)
			if (tmp(args...) && can_canceled)
				return true;
		for (auto& tmp : lower_avg_priorihty)
			if (tmp(args...) && can_canceled)
				return true;
		for (auto& tmp : low_priorihty)
			if (tmp(args...) && can_canceled)
				return true;
		return false;
	}
};