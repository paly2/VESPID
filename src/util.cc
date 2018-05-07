#include <cstdlib>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <SDL_timer.h>

#include "gpio.hh"
#include "util.hh"

namespace Conf {
	long getInt(const char *name) {
		char *value, *endptr;
		long result;

		value = getenv(name);
		if (value == NULL)
			throw ConfException(name);
		result = strtol(value, &endptr, 10);
		if (endptr == value)
			throw ConfException(name);

		return result;
	}

	long getInt(const char *name, long defau) {
		try {
			return getInt(name);
		} catch (ConfException &ex) {
			return defau;
		}
	}

	double getDouble(const char *name) {
		char *value, *endptr;
		double result;

		value = getenv(name);
		if (value == NULL)
			throw ConfException(name);
		result = strtod(value, &endptr);
		if (endptr == value)
			throw ConfException(name);

		return result;
	}

	double getDouble(const char *name, double defau) {
		try {
			return getDouble(name);
		} catch (ConfException &ex) {
			return defau;
		}
	}
}

namespace Thread {
	ConsumerTracker::ConsumerTracker(int n) : n_consumers(n) {
		mutex = SDL_CreateMutex();
		newval_table = new bool[n_consumers];
		for (int i = 0 ; i < n_consumers ; ++i)
			newval_table[i] = false;
	}

	ConsumerTracker::~ConsumerTracker() {
		delete[] newval_table;
		SDL_DestroyMutex(mutex);
		mutex = NULL;
	}

	bool ConsumerTracker::getSingle(int src_id) {
		bool v;
		SDL_LockMutex(mutex);
		v = newval_table[src_id];
		SDL_UnlockMutex(mutex);
		return v;
	}

	void ConsumerTracker::setSingleFalse(int src_id) {
		SDL_LockMutex(mutex);
		newval_table[src_id] = false;
		SDL_UnlockMutex(mutex);
	}

	void ConsumerTracker::setAllTrue() {
		SDL_LockMutex(mutex);
		for (int i = 0 ; i < n_consumers ; ++i)
			newval_table[i] = true;
		SDL_UnlockMutex(mutex);
	}

	void ThreadBase::destruct() {
		if (m_thread == NULL)
			return; // No yet launched

		if (SDL_SemValue(m_end_sem) == 0) {
			SDL_SemPost(m_kill_sem);
			SDL_SemWait(m_end_sem);
		}

		SDL_DestroySemaphore(m_init_sem);
		SDL_DestroySemaphore(m_kill_sem);
		SDL_DestroySemaphore(m_end_sem);
	}

	void ThreadBase::setFrequency(int freq) {
		if (freq == 0)
			m_ms_wait = 0;
		else
			m_ms_wait = 1000.0 / (double) freq;
	}

	void ThreadBase::launch(const char *name) {
		m_init_sem = SDL_CreateSemaphore(0);
		m_kill_sem = SDL_CreateSemaphore(0);
		m_end_sem = SDL_CreateSemaphore(0);
		construct();

		m_thread = SDL_CreateThread(ThreadBase::threadBaseFunc, name, (void*) this);
		SDL_DetachThread(m_thread);
		SDL_SemWait(m_init_sem);
		checkDeath();
	}

	void ThreadBase::checkDeath() {
		if (SDL_SemValue(m_end_sem) > 0)
			std::rethrow_exception(m_except);
	}

	int ThreadBase::threadBaseFunc(void *data) {
		ThreadBase *thread = (ThreadBase*) data;
		try {
			thread->onStart();
		} catch (std::exception &e) {
			thread->m_except = std::current_exception();
			thread->onEnd();
			SDL_SemPost(thread->m_end_sem);
			SDL_SemPost(thread->m_init_sem);
			return -1;
		}

		SDL_SemPost(thread->m_init_sem);

		try {
			int ms_wait = 0;
			while (SDL_SemWaitTimeout(thread->m_kill_sem, ms_wait) == SDL_MUTEX_TIMEDOUT) {
				unsigned int start_ticks = Time::getTicks();

				thread->loop();

				int remain = thread->m_ms_wait - (Time::getTicks() - start_ticks);
				ms_wait = (remain > 0) ? remain : 0;
			}
		} catch (std::exception &e) {
			thread->m_except = std::current_exception();
			thread->onEnd();
			SDL_SemPost(thread->m_end_sem);
			return -1;
		}

		thread->onEnd();

		SDL_SemPost(thread->m_end_sem);
		return 0;
	}
}

namespace Time {
	unsigned int getTicks() {
		return SDL_GetTicks();
	}

	void delay(unsigned int ms) {
		SDL_Delay(ms);
	}
}
