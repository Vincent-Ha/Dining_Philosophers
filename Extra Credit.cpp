//-----------------------------------------------------------------------------
// File: Program.h
//
// Desc: Dining Philosophers sample.
//		 
//		 Demonstrates how to use Monitor object (lock) to protect critical section.
//		 
//		 Scenario is such that there are 5 philosophers and 5 tools on each side of a philosopher.
//		 Each philosopher can only take one tool on the left and one on the right.
//		 One of the philosophers must wait for a tool to become available because whoever grabs
//		 a tool will hold it until he eats and puts the tool back on the table.
//		 
//		 Application of the pattern could be transferring money from account A to account B.
//		 Important here is to pass locking objects always in the same (increasing) order.
//		 If the order is mixed you would encounter random deadlocks at runtime.
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <future>
#include <vector>
#include <string>
#include "ConsoleColor.h"
using namespace std;

/*
At a round table sit five philosophers who alternate between thinking
and eating from a large bowl of rice at random intervals. Unfortunately
for the philosophers, there are only five chopsticks at the table, and
before the philosophers can begin eating, they must acquire a pair of
chopsticks. Since the chopsticks are shared between the philosophers,
access to the chopsticks must be protected, but if care isn't taken,
concurrency problems arise. Most notably, starvation via deadlock can
occur: if each philosopher picks up a chopstick as soon as it is available
and waits indefinitely for the other, eventually they will all end up
holding only one chopstick with no chance of acquiring another.

The Dining Philosophers problem is typically represented in code by a
thread for each philosopher and some form of shared state used to
represent each of the chopsticks. Straightforward solutions to this
problem often involve introducing a waiter entity to coordinate access
to the chopsticks, introducing lock ordering heuristics, and manually
working with threading APIs, critical sections, semaphores, or monitors
to manage the state.
*/
ostream& (*cstream[])(ostream&) =
{
	blue, green, red, yellow, white
};

class Lock {
private:
	std::atomic_flag value = ATOMIC_FLAG_INIT;

public:
	Lock() {};
	void lock();
	void unlock();
};

Lock coutLock;

class Chopstick
{
private:
	Lock lck;
	string gettingMessage(int philosopherNumber, int chopstickNumber);
public:
	Chopstick() {};
	Lock& getLock() { return lck; };
	string getMessage(int philosopherNumber, int chopstickNumber);
};

void eating(string firstMessage, string secondMessage, int philosopherNumber) {
	this_thread::yield();
	string pe = "Philosopher " + to_string(philosopherNumber) + " eats.\n";
	{
		coutLock.lock();
		cout << cstream[philosopherNumber - 1] << pe;
		coutLock.unlock();
	}
}

void Diner()
{
	auto eat = [](Chopstick* leftChopstick, Chopstick* rightChopstick, int philosopherNumber, int leftChopstickNumber, int rightChopstickNumber)
	{
		if (leftChopstick == rightChopstick)
			throw exception("Left and right chopsticks should not be the same!");

		// ensures there are no deadlocks
		leftChopstick->getLock().lock();
		rightChopstick->getLock().lock();

		eating(leftChopstick->getMessage(philosopherNumber, leftChopstickNumber), rightChopstick->getMessage(philosopherNumber, rightChopstickNumber), philosopherNumber);

		leftChopstick->getLock().unlock();
		rightChopstick->getLock().unlock();
		string done = "done";
		return done;
	};

	static const int numPhilosophers = 5;

	// 5 utencils on the left and right of each philosopher, 
	// used them to acquire locks.
	vector< unique_ptr<Chopstick> > chopsticks(numPhilosophers);

	for (int i = 0; i < numPhilosophers; ++i)
	{
		auto chopstick = unique_ptr<Chopstick>(new Chopstick());
		chopsticks[i] = move(chopstick);
	}

	// Create philosophers, each of 5 tasks represents one philosopher.
	vector<future <string>> philosopher(numPhilosophers);

	// Initialize first philosopher
	philosopher[0] = async(eat,
		chopsticks[0].get(),	// left chopstick:  #1
		chopsticks[numPhilosophers - 1].get(),	// right chopstick: #5
		0 + 1,		// philosopher number
		1,
		numPhilosophers
	);

	// Start remaining philosophers
	for (int i = 1; i < numPhilosophers; ++i)
	{
		philosopher[i] = (async(eat,
			chopsticks[i - 1].get(),	// left chopstick
			chopsticks[i].get(),		// right chopstick
			i + 1,						// philosopher number
			i,
			i + 1
		)
			);
	}

	// Start Dining
	for_each(philosopher.begin(), philosopher.end(), [](future <string>& a) { a.get(); });
}

void main()
{
	Diner();
}

void Lock::lock()
{
	while (value.test_and_set(std::memory_order_acquire)) {
	};
}

void Lock::unlock()
{
	value.clear(std::memory_order_release);
};

string Chopstick::gettingMessage(int philosopherNumber, int chopstickNumber) {
	string message = "   Philosopher " + to_string(philosopherNumber) + " picked " + to_string(chopstickNumber) + " chopstick.\n";
	coutLock.lock();
	cout << cstream[philosopherNumber - 1] << message.c_str();
	coutLock.unlock();
	return message;
}

string Chopstick::getMessage(int philosopherNumber, int chopstickNumber) {
	future <string> temp_future = async(&Chopstick::gettingMessage, this, philosopherNumber, chopstickNumber);
	return temp_future.get();
}
