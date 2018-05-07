#pragma once

#include <exception>
#include <cstring>
#include <SDL_thread.h>
#include <SDL_mutex.h>

namespace Conf {
	struct ConfException : public std::exception {
		ConfException(const char* v_name) : name(v_name) {}
		const char* what() const noexcept {
			static char str[200] = "Error reading environment variable ";
			strncat(str, name, 199 - strlen(str));
			return str;
		}

		const char *name;
	};

	long getInt(const char* name);
	long getInt(const char* name, long defau);
	double getDouble(const char* name);
	double getDouble(const char* name, double defau);
}

namespace Thread {
	class ConsumerTracker {
	public:
		ConsumerTracker(int n);
		~ConsumerTracker();
		bool getSingle(int src_id);
		void setSingleFalse(int src_id);
		void setAllTrue();

	private:
		int n_consumers;
		SDL_mutex *mutex;
		bool *newval_table;
	};

	class ThreadBase {
	public:
		virtual void onStart() = 0;
		virtual void onEnd() = 0;
		virtual void loop() = 0;
		virtual void construct() {}

		// The inherited class in responsible for calling this function
		// in its destructor.
		void destruct();
		void launch(const char *name);
		// If death happened, this function throws an exception.
		void checkDeath();
		// In runs per second, 0 = maximum
		void setFrequency(int freq);

	private:
		int m_ms_wait = 0;
		SDL_Thread *m_thread = NULL;
		SDL_sem *m_kill_sem = NULL;
		SDL_sem *m_end_sem = NULL;
		SDL_sem *m_init_sem = NULL;
		std::exception_ptr m_except;

		static int threadBaseFunc(void *data);
	};
}

namespace Time {
	unsigned int getTicks();
	void delay(unsigned int ms);
}
