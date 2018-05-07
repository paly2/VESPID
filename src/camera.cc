#include <raspicam/raspicam_cv.h>
#include <cxcore.hpp>
#include <SDL_thread.h>
#include <SDL_mutex.h>

#include "camera.hh"
#include "util.hh"

namespace Camera {
	Camera::Camera() : m_newimage_tracker(CAMERA_CLASER_ONSUMERS) {
		m_thread.launch("CameraThread");
	}

	bool Camera::newImage(int src_id) {
		if (m_newimage_tracker.getSingle(src_id)) {
			return true;
		} else if (SDL_SemTryWait(m_thread.newimage_sem) != SDL_MUTEX_TIMEDOUT) {
			m_newimage_tracker.setAllTrue();
			return true;
		}

		return false;
	}

	void Camera::waitForImage(int src_id) {
		if (m_newimage_tracker.getSingle(src_id))
			return;

		SDL_SemWait(m_thread.newimage_sem);
		m_newimage_tracker.setAllTrue();
	}

	void Camera::retrieve(cv::Mat &image, int src_id) {
		SDL_LockMutex(m_thread.mutex);
		image = m_thread.image;
		SDL_UnlockMutex(m_thread.mutex);
		m_newimage_tracker.setSingleFalse(src_id);
	}

	void CameraThread::construct() {
		mutex = SDL_CreateMutex();
		newimage_sem = SDL_CreateSemaphore(0);
	}

	CameraThread::~CameraThread() {
		destruct();

		if (newimage_sem != NULL)
			SDL_DestroySemaphore(newimage_sem);
		if (mutex != NULL)
			SDL_DestroyMutex(mutex);
	}

	void CameraThread::onStart() {
		camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
		if (!camera.open())
			throw CameraException();
	}

	void CameraThread::onEnd() {
		camera.release();
	}

	void CameraThread::loop() {
		camera.grab();
		SDL_LockMutex(mutex);
		camera.retrieve(image);
		SDL_UnlockMutex(mutex);
		if (SDL_SemValue(newimage_sem) == 0)
			SDL_SemPost(newimage_sem);
	}
}
