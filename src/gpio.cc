#include <SDL_mutex.h>
#include <pigpiod_if2.h>

#include "gpio.hh"
#include "image.hh"
#include "util.hh"

// The number of times the GPIO thread should run per second
// The GPIO thread must run faster than all other threads.
// (It is very lightweight so that shouldn't be a problem)
#define GPIO_FREQUENCY 50

namespace GPIO {
	GPIO::GPIO(Image::NNManager *nn_manager) {
		m_thread.nn_manager = nn_manager;
		m_thread.laser_pin = Conf::getInt("LASER_RECEPTOR_PIN");
		m_thread.servo_pin = Conf::getInt("SERVO_PIN");
		m_thread.servo_death = Conf::getInt("SERVO_DEATH");
		m_thread.servo_life = Conf::getInt("SERVO_LIFE");
		m_thread.delay_empty = Conf::getInt("EMPTY_DELAY", 2000);
		m_thread.min_prob = Conf::getDouble("MIN_PROB", 0.7);

		m_thread.setFrequency(GPIO_FREQUENCY);
		m_thread.launch("GPIOThread");
	}

	servoState GPIO::getServoState() {
		return m_thread.getServoState();
	}

	laserState GPIO::getLaserState() {
		return m_thread.getLaserState();
	}

	void GPIO::simLaserOn() {
		m_thread.simLaserOn();
	}

	void GPIO::simLaserOff() {
		m_thread.simLaserOff();
	}

	void GPIOThread::construct() {
		mutex = SDL_CreateMutex();
	}

	GPIOThread::~GPIOThread() {
		destruct();

		if (mutex != NULL)
			SDL_DestroyMutex(mutex);
	}

	void GPIOThread::onStart() {
		if ((gpio_pi = pigpio_start(NULL, NULL)) < 0)
			throw GPIOException();
		if (set_mode(gpio_pi, servo_pin, PI_OUTPUT) != 0)
			throw GPIOException();
		if (set_mode(gpio_pi, laser_pin, PI_INPUT) != 0)
			throw GPIOException();
		if (set_servo_pulsewidth(gpio_pi, servo_pin, servo_life) != 0)
			throw GPIOException();
	}

	void GPIOThread::onEnd() {
		if (gpio_pi >= 0)
			pigpio_stop(gpio_pi);
	}

	void GPIOThread::loop() {
		/// Retrieve laserState
		SDL_LockMutex(mutex);
		if (!gpio_read(gpio_pi, laser_pin) || simlaser) {
			laser_state = LASER_ON;
			active = true;
		} else {
			laser_state = LASER_OFF;
		}
		SDL_UnlockMutex(mutex);

		/// Active mode management
		if (active)
			activeLoop();
	}

	void GPIOThread::activeLoop() {
		if (empty_timer != NULL) {
			// Empty timer stage:
			// If all images are empty for 2 seconds, switch back
			// to waiting mode.

			// Note: we can safely read laser_state without locking
			// the mutex because no other thread writes to this value.
			// This is enforced by the absence of public method to change
			// these values.
			if (laser_state == LASER_ON)
				empty_timer->reset();

			empty_timer->step();
			if (empty_timer->getEmptyTime() >= delay_empty) {
				setServo(SERVO_LIFE);
				active = false;
				delete empty_timer;
				empty_timer = NULL;
			}
		} else if (image_processor != NULL) {
			// Image processing stage:
			// Analyse five images and check the average results
			image_processor->step();
			if (image_processor->getProcessedNumber() >= 5) {
				Image::nnResult result;
				result = image_processor->getAverageResult();
				delete image_processor;
				image_processor = NULL;

				if (result.asian_prob > min_prob) {
					// Start empty timer stage
					setServo(SERVO_DEATH);
					empty_timer = new EmptyTimer(nn_manager);
				} else {
					// Unvalidated.
					active = false;
				}
			}
		} else {
			// First run, stat image processing stage
			image_processor = new ImageProcessor(nn_manager);
		}
	}

	void GPIOThread::setServo(servoState p_servo_state) {
		set_servo_pulsewidth(gpio_pi, servo_pin, (p_servo_state == SERVO_LIFE) ? servo_life : servo_death);
		SDL_LockMutex(mutex);
		servo_state = p_servo_state;
		SDL_UnlockMutex(mutex);
	}

	servoState GPIOThread::getServoState() {
		servoState ret;
		SDL_LockMutex(mutex);
		ret = servo_state;
		SDL_UnlockMutex(mutex);
		return ret;
	}

	laserState GPIOThread::getLaserState() {
		laserState ret;
		SDL_LockMutex(mutex);
		ret = laser_state;
		SDL_UnlockMutex(mutex);
		return ret;
	}

	void GPIOThread::simLaserOn() {
		SDL_LockMutex(mutex);
		simlaser = true;
		SDL_UnlockMutex(mutex);
	}

	void GPIOThread::simLaserOff() {
		SDL_LockMutex(mutex);
		simlaser = false;
		SDL_UnlockMutex(mutex);
	}

	ImageProcessor::ImageProcessor(Image::NNManager *nn_manager) {
		m_nn_manager = nn_manager;
	}

	void ImageProcessor::step() {
		if (m_nn_manager->newResult(RESULTS_CLASER_ONSUMER_GPIO_ID))
			m_results.push_back(m_nn_manager->getResult(RESULTS_CLASER_ONSUMER_GPIO_ID));
	}

	unsigned int ImageProcessor::getProcessedNumber() {
		return m_results.size();
	}

	Image::nnResult ImageProcessor::getAverageResult() {
		Image::nnResult sum = {0, 0, 0};

		for (std::vector<Image::nnResult>::iterator it = m_results.begin() ; it != m_results.end() ; ++it) {
			sum.asian_prob += it->asian_prob;
			sum.european_prob += it->european_prob;
			sum.empty_prob += it->empty_prob;
		}

		sum.asian_prob /= (double) m_results.size();
		sum.european_prob /= (double) m_results.size();
		sum.empty_prob /= (double) m_results.size();
		return sum;
	}

	EmptyTimer::EmptyTimer(Image::NNManager *nn_manager) {
		m_nn_manager = nn_manager;
		m_start_ticks = Time::getTicks();
	}

	void EmptyTimer::step() {
		if (m_nn_manager->newResult(RESULTS_CLASER_ONSUMER_GPIO_ID)) {
			Image::nnResult result = m_nn_manager->getResult(RESULTS_CLASER_ONSUMER_GPIO_ID);
			if (result.empty_prob < result.asian_prob || result.empty_prob < result.european_prob)
				reset();
		}
	}

	void EmptyTimer::reset() {
		m_start_ticks = Time::getTicks();
	}

	int EmptyTimer::getEmptyTime() {
		return Time::getTicks() - m_start_ticks;
	}
}
