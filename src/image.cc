#include <string>
#include <SDL_surface.h>
#include <SDL_thread.h>
#include <cxcore.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <lua.hpp>

#include "image.hh"
#include "camera.hh"
#include "cmake_config.h"

#define TESTSCRIPT SHAREDIR "/test.lua"
#define IMG_SAVE_PATH "/tmp/hornet.ppm"

namespace Image {
	void NNManagerThread::construct() {
		newresult_sem = SDL_CreateSemaphore(0);
		mutex = SDL_CreateMutex();
	}

	NNManagerThread::~NNManagerThread() {
		destruct();

		if (newresult_sem != NULL)
			SDL_DestroySemaphore(newresult_sem);
		if (mutex != NULL)
			SDL_DestroyMutex(mutex);
	}

	void NNManagerThread::onStart() {
		L = luaL_newstate();
		if (L == NULL)
			throw LuaException(LUA_ERRMEM);

		luaL_openlibs(L);

		thread_state = lua_newthread(L);

		int err;
		if ((err = luaL_loadfile(thread_state, TESTSCRIPT)) != 0)
			throw LuaException(err);

		// Push the thread arguments
		lua_pushstring(thread_state, IMG_SAVE_PATH);
		if ((err = lua_resume(thread_state, 1)) > 1)
			throw LuaException(err, std::string(lua_tostring(thread_state, -1)));
	}

	void NNManagerThread::onEnd() {
		lua_close(L);
	}

	void NNManagerThread::loop() {
		camera->waitForImage(CAMERA_CLASER_ONSUMER_PROCESSING_ID);
		cv::Mat image;
		camera->retrieve(image, CAMERA_CLASER_ONSUMER_PROCESSING_ID);

		cv::Mat resized;
		resizeImageForDB(image, resized);
		imwrite(IMG_SAVE_PATH, resized);

		// Resume the Lua thread
		int err;
		if ((err = lua_resume(thread_state, 0)) > 1)
			throw LuaException(err, std::string(lua_tostring(thread_state, -1)));

		nnResult tmp_result;
		// Get the results
		lua_getfield(thread_state, 1, "empty");
		tmp_result.empty_prob = lua_tonumber(thread_state, 2);
		lua_pop(thread_state, 1);
		lua_getfield(thread_state, 1, "asian");
		tmp_result.asian_prob = lua_tonumber(thread_state, 2);
		lua_pop(thread_state, 1);
		lua_getfield(thread_state, 1, "european");
		tmp_result.european_prob = lua_tonumber(thread_state, 2);
		lua_pop(thread_state, 2);

		SDL_LockMutex(mutex);
		result = tmp_result;
		SDL_UnlockMutex(mutex);
		SDL_SemPost(newresult_sem);
	}

	NNManager::NNManager(Camera::Camera *camera) : m_newresult_tracker(RESULTS_CLASER_ONSUMERS) {
		m_thread.camera = camera;
		m_thread.launch("ImageProcessingThread");
	}

	bool NNManager::newResult(int src_id) {
		m_thread.checkDeath();

		if (m_newresult_tracker.getSingle(src_id)) {
			return true;
		} else if (SDL_SemTryWait(m_thread.newresult_sem) != SDL_MUTEX_TIMEDOUT) {
			m_newresult_tracker.setAllTrue();
			return true;
		}

		return false;
	}

	nnResult NNManager::getResult(int src_id) {
		m_thread.checkDeath();

		nnResult result;
		SDL_LockMutex(m_thread.mutex);
		result = m_thread.result;
		SDL_UnlockMutex(m_thread.mutex);

		m_newresult_tracker.setSingleFalse(src_id);
		return result;
	}

	void resizeImageForDB(const cv::Mat &src, cv::Mat &dst) {
		// This function stretches the image so it fits the horizontal
		// resolution first, then crops the top and bottom parts to it
		// fits the vertical resolution.
		// It makes no checks about the source image, no be careful.
		cv::Mat resized;
		double ratio = (double) DB_RESIZED_IMAGE_WIDTH / (double) src.cols;
		cv::resize(src, resized, cv::Size(), ratio, ratio, cv::INTER_AREA);

		cv::Rect roi(0, (resized.rows - DB_RESIZED_IMAGE_HEIGHT) / 2, DB_RESIZED_IMAGE_WIDTH, DB_RESIZED_IMAGE_HEIGHT);
		dst = resized(roi);
	}

	void resizeImageForScreen(const cv::Mat &src, cv::Mat &dst, int width, int height, int &x_pos, int &y_pos) {
		double x_ratio, y_ratio;
		x_ratio = (double) width / (double) src.cols;
		y_ratio = (double) height / (double) src.rows;

		y_pos = x_pos = 0;

		if (x_ratio < y_ratio) {
			cv::resize(src, dst, cv::Size(), x_ratio, x_ratio);
			y_pos = (height - dst.rows) / 2;
		} else {
			cv::resize(src, dst, cv::Size(), y_ratio, y_ratio);
			x_pos = (width - dst.cols) / 2;
		}
	}

	SDL_Surface* matToSurface(const cv::Mat &image) {
		// This function assumes an RGB image with a 24-bits depth.
		SDL_Surface *surface = SDL_CreateRGBSurfaceFrom((void*) image.data,
			image.cols,
			image.rows,
			24,
			image.cols * 3,
			0xff0000, 0x00ff00, 0x0000ff, 0);
		return surface;
	}
}
