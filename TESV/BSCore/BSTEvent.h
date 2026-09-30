#pragma once

template <class Event>
class BSTEventSource
{
public:
	void Notify(const Event& arEvent);
};
