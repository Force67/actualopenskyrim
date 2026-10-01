#pragma once

template <class Event>
class BSTEventSink;

template <class Event>
class BSTEventSource
{
public:
	void Notify(const Event& arEvent);
	void RegisterSink(BSTEventSink<Event>* apSink);
	void UnregisterSink(BSTEventSink<Event>* apSink);
};
