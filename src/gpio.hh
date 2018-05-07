#pragma once

#include <exception>
#include <vector>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <cxcore.hpp>

#include "image.hh"

namespace GPIO {
	// The special NLASER_ONE value is used by the GUI
	enum servoState { SERVO_DEATH, SERVO_LIFE, SERVO_NLASER_ONE };
	enum laserState { LASER_ON, LASER_OFF, LASER_NLASER_ONE };

	struct GPIOException : public std::exception {
		const char* what() const noexcept {
			return "GPIO error.";
		}
	};

	class ImageProcessor {
	public:
		ImageProcessor(Image::NNManager *nn_manager);
		void step();
		unsigned int getProcessedNumber();
		Image::nnResult getAverageResult();
	private:
		Image::NNManager *m_nn_manager;
		std::vector<Image::nnResult> m_results;
	};

	class EmptyTimer {
	public:
		EmptyTimer(Image::NNManager *nn_manager);
		void step();
		int getEmptyTime();
		void reset();
	private:
		unsigned int m_start_ticks;
		Image::NNManager *m_nn_manager;
	};

	class GPIOThread : public Thread::ThreadBase {
	public:
		virtual void onStart();
		virtual void onEnd();
		virtual void loop();
		virtual void construct();
		~GPIOThread();

		servoState getServoState();
		laserState getLaserState();
		void simLaserOn();
		void simLaserOff();

		// Settings (read-only)
		long laser_pin;
		long servo_pin;
		long servo_death;
		long servo_life;
		long delay_empty;
		double min_prob;

		// NNManager object
		Image::NNManager *nn_manager;

	private:
		void activeLoop();
		void setServo(servoState servo_satte);

		// Information passing with main thread
		SDL_mutex *mutex = NULL;
		servoState servo_state = SERVO_LIFE;
		laserState laser_state = LASER_OFF;
		bool simlaser = false;

		bool active = false;
		ImageProcessor *image_processor = NULL;
		EmptyTimer *empty_timer = NULL;

		int gpio_pi = -1;
	};

	class GPIO {
	public:
		GPIO(Image::NNManager *nn_manager);

		servoState getServoState();
		laserState getLaserState();
		void simLaserOn();
		void simLaserOff();

	private:
		GPIOThread m_thread;
	};
}
