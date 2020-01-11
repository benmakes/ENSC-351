//===================================================================================
// Name        : posixThread.cpp
// Author(s)   :
//			   : Craig Scratchley
//			   : Eton Kan
// Version     : November 2019
// Copyright   : Copyright (C) 2019, Craig Scratchley and Eton Kan
// Description : Set schedule parameters, priority and policy for current/given thread
//===================================================================================

#include <sched.h>
#include <pthread.h>
#include <system_error>

namespace pthreadSupport
{
	//Overloaded function: get policy and sched_param (including priority) for the current thread
	int getSchedParam(int *policy, sched_param *sch)
	{
		if(pthread_getschedparam(pthread_self(), policy, sch)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	//Overloaded function: get policy and sched_param (including priority) for the given thread
	int getSchedParam(pthread_t th, int *policy, sched_param *sch)
	{
		if(pthread_getschedparam(th, policy, sch)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	//Overloaded function: set sched_param (including priority) and policy for the current thread
	int setSchedParam(int policy, sched_param sch)
	{
		if(pthread_setschedparam(pthread_self(), policy, &sch)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	//Overloaded function: set sched_param (including priority) and policy for the given thread
	int setSchedParam(pthread_t th, int policy, sched_param sch)
	{
		if(pthread_setschedparam(th, policy, &sch)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	//Overloaded function: set priority for the current thread
	int setSchedPrio(int priority)
	{
		if(pthread_setschedprio(pthread_self(), priority)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	//Overloaded function: set priority for the given thread
	int setSchedPrio(pthread_t th, int priority)
	{
		if(pthread_setschedprio(th, priority)) {
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return 0;
	}

	int get_priority_max(int policy)
	{
		int max = sched_get_priority_max(policy);
		if(max == -1){
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return max;
	}

	int get_priority_min(int policy)
	{
		int min = sched_get_priority_min(policy);
		if(min == -1){
			throw std::system_error(errno, std::system_category());
			return -1;
		}
		return min;
	}
}


