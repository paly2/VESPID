#pragma once

#include <queue>
#include <cstdio>
#include <string>
#include <SDL_surface.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <cxcore.hpp>
#include <lua.hpp>

#include "camera.hh"
#include "util.hh"

#define DB_RESIZED_IMAGE_WIDTH 32
#define DB_RESIZED_IMAGE_HEIGHT 16

#define RESULTS_CLASER_ONSUMERS 2
#define RESULTS_CLASER_ONSUMER_MAIN_ID 0
#define RESULTS_CLASER_ONSUMER_GPIO_ID 1

namespace Image {
	struct LuaException : public std::exception {
		LuaException(int errcode, std::string p_msg = "") : err(errcode), msg(p_msg) {}

		const char* what() const noexcept {
			static char ret[1000];
			switch (err) {
			case LUA_ERRSYNTAX:
				return "Lua error: syntax error";
			case LUA_ERRMEM:
				return "Lua error: not enough memory.";
			case LUA_ERRRUN:
				snprintf(ret, 1000, "Lua error: runtime error: %s", msg.c_str());
				return ret;
			case LUA_ERRERR:
				return "Lua error: error while running the message handler.";
			case LUA_ERRFILE:
				return "Lua error: file error.";
			default:
				snprintf(ret, 1000, "Lua error: unknown error: %d", err);
				return ret;
			}
		}

		int err;
		std::string msg;
	};

	struct nnResult {
		double empty_prob;
		double asian_prob;
		double european_prob;
	};

	class NNManagerThread : public Thread::ThreadBase {
	public:
		virtual void onStart();
		virtual void onEnd();
		virtual void loop();
		virtual void construct();
		~NNManagerThread();

		SDL_sem *newresult_sem = NULL;
		SDL_mutex *mutex = NULL;
		nnResult result;
		Camera::Camera *camera;

	private:
		lua_State *L;
		lua_State *thread_state;
	};

	class NNManager {
	public:
		NNManager(Camera::Camera *camera);
		bool newResult(int src_id);
		nnResult getResult(int src_id);
	private:
		NNManagerThread m_thread;
		Thread::ConsumerTracker m_newresult_tracker;
	};

	void resizeImageForDB(const cv::Mat &src, cv::Mat &dst);
	void resizeImageForScreen(const cv::Mat &src, cv::Mat &dst, int width, int height, int &x_pos, int &y_pos);
}
