#pragma once

#include <exception>
#include <raspicam/raspicam_cv.h>
#include <cxcore.hpp>
#include <SDL_thread.h>
#include <SDL_mutex.h>

#include "util.hh"

#define IMAGE_SAVE_PATH "/tmp/hornetimg.ppm"

#define CAMERA_CLASER_ONSUMERS 2
#define CAMERA_CLASER_ONSUMER_MAIN_ID 0
#define CAMERA_CLASER_ONSUMER_PROCESSING_ID 1

namespace Camera {
	struct CameraException : public std::exception {
		const char* what() const noexcept {
			return "Failed to open camera.";
		}
	};

	class CameraThread : public Thread::ThreadBase {
	public:
		virtual void onStart();
		virtual void onEnd();
		virtual void loop();
		virtual void construct();
		~CameraThread();
		cv::Mat image;
		SDL_mutex *mutex = NULL;
		SDL_sem *newimage_sem = NULL;

	private:
		raspicam::RaspiCam_Cv camera;
	};

	class Camera {
	public:
		Camera();

		// All these functions are thread-safe.
		bool newImage(int src_id);
		void waitForImage(int src_id);
		void retrieve(cv::Mat &image, int src_id);

	private:
		Thread::ConsumerTracker m_newimage_tracker;
		CameraThread m_thread;

		static int camThread(void *thread_data);
	};
}
