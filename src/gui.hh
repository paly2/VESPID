#pragma once

#include <exception>
#include <string>
#include <cstdio>
#include <vector>
#include <SDL.h>
#include <SDL_surface.h>
#include <SDL_error.h>
#include <SDL_ttf.h>
#include <SDL_render.h>
#include <cxcore.hpp>

#include "image.hh"
#include "gpio.hh"
#include "cmake_config.h"

#define GUI_TTF_FONT   SHAREDIR "/fonts/freefont/FreeSans.ttf"
#define GUI_TTF_PTSIZE 16
#define SERVO_POS_X    50
#define SERVO_POS_Y    50
#define LASER_POS_X    50
#define LASER_POS_Y    100
#define RESULT_POS_X   50
#define RESULT_POS_Y   150
#define CAPTURE_MODE_POS_X 50
#define CAPTURE_MODE_POS_Y 50
#define CAPTURE_PATH_POS_X 50
#define CAPTURE_PATH_POS_Y 100
#define FREQ_POS_X    -50
#define FREQ_POS_Y    50
#define COMPILE_DATE_POS_X -20
#define COMPILE_DATE_POS_Y -20

#define TEXTURE(type, name, x, y) type name = type(x, y, this);

namespace GUI {
	struct SDLException : public std::exception {
		SDLException(const char *err) : s(err) {}
		const char* what() const noexcept {
			static char err[200] = "SDL Error: ";
			sprintf(err, "SDL Error: %s: %s", s, SDL_GetError());
			return err;
		}

		const char *s;
	};

	struct Event {
		bool quit              = false;
		bool captureImage      = false;
		bool captureMode       = false;
		// LaserOn is used to simulate a laser enabling using keyboard.
		bool laserOn           = false;
		bool laserOff          = false;
	};

	enum captureMode { NORMAL, CAPTURE_EMPTY, CAPTURE_ASIAN, CAPTURE_EUROPEAN };

	/* This is somewhat uncommon.
	 * This class is meant to be used as base class for anonymous structs
	 * containing textures in the GUI class. Textures must be declared
	 * with this as last argument (you may use the TEXTURE macro).
	 * This allows convenient access to textures for the GUI class while keeping
	 * it easy to iterate over them, and avoiding the resource overhead caused
	 * by, e.g., a map.
	 * I still feel like I'm missing a feature that would allow implementing
	 * it in a more straightforward way.
	 * Probably looks unmaintainable. Not *that* much.
	 */
	class Texture;
	struct TextureSet {
	public:
		void init(SDL_Renderer *renderer, TTF_Font *font);
		void draw();
		void _addTexture(Texture* texture);

	private:
		std::vector<Texture*> m_textures;
	};

	// Base Texture
	class Texture {
	public:
		Texture(int x, int y, TextureSet *parent_set);
		~Texture();
		void setRenderer(SDL_Renderer *renderer);
		void setFont(TTF_Font *font);
		void draw();
		SDL_Renderer *renderer = NULL;

	protected:
		bool isInitialized();
		void recreateFromText(const char *str, SDL_Color color);
		void recreateFromSurface(SDL_Surface *surface);
		void recreateEmpty(unsigned int format, int w, int h);
		void updateFromData(void *data, int pitch);
		void destroy();
		void setXY(int x, int y);

	private:
		int x = 0, y = 0;
		SDL_Texture *texture = NULL;
		SDL_Rect rect = {0, 0, 0, 0};
		TTF_Font *font;
	};

	// Derived Textures

	class TextureImage : public Texture {
	public:
		using Texture::Texture;
		void updateFromImage(const cv::Mat &image);
	};

	class TextureFreq : public Texture {
	public:
		TextureFreq(int x, int y, TextureSet *parent_set);
		void redrawing();
	private:
		unsigned int m_last_freq_ticks = 0;
		unsigned int m_freq_ms_sum = 0;
		unsigned int m_freq_ms_number = 0;
	};

	class TextureCompileDate : public Texture {
	public:
		using Texture::Texture;
		void generate();
	};

	class TextureServo : public Texture {
	public:
		using Texture::Texture;
		void setServoState(GPIO::servoState servo_state);
	private:
		GPIO::servoState m_servo_state = GPIO::SERVO_NLASER_ONE;
	};

	class TextureLaser : public Texture {
	public:
		using Texture::Texture;
		void setLaserState(GPIO::laserState laser_state);
	private:
		GPIO::laserState m_laser_state = GPIO::LASER_NLASER_ONE;
	};

	class TextureNNResult : public Texture {
	public:
		using Texture::Texture;
		void setNNResult(Image::nnResult result);
	};

	class TextureCaptureMode : public Texture {
	public:
		using Texture::Texture;
		void setMode(captureMode mode);
	};

	class TextureCapturePath : public Texture {
	public:
		using Texture::Texture;
		void setCapturePath(std::string path);
	};

	class GUI {
	public:
		GUI();
		~GUI();

		// Normal mode-specific
		void updateServo(GPIO::servoState servo_state);
		void updateLaser(GPIO::laserState laser_state);
		void updateNNResult(Image::nnResult result);

		// Capture mode-specific
		void setMode(captureMode mode);
		void updateCapturePath(std::string path);

		// General
		Event pollEvent();
		void updateImage(const cv::Mat &image);
		void redraw();

	private:
		SDL_Window* m_window = NULL;
		SDL_Renderer* m_renderer = NULL;
		TTF_Font* m_font = NULL;

		captureMode m_mode = NORMAL;
		// Textures are drawn from first to last.
		// If you want to add a texture, all you have to do is to add
		// a such line.
		struct : TextureSet {
			TEXTURE(TextureImage, image, 0, 0)
			TEXTURE(TextureFreq, freq, FREQ_POS_X, FREQ_POS_Y)
			TEXTURE(TextureCompileDate, compile_date, COMPILE_DATE_POS_X, COMPILE_DATE_POS_Y)
		} m_general_textures;
		struct : TextureSet {
			TEXTURE(TextureServo, servo, SERVO_POS_X, SERVO_POS_Y)
			TEXTURE(TextureLaser, laser, LASER_POS_X, LASER_POS_Y)
			TEXTURE(TextureNNResult, result, RESULT_POS_X, RESULT_POS_Y)
		} m_normal_mode_textures;
		struct : TextureSet {
			TEXTURE(TextureCaptureMode, capture_mode, CAPTURE_MODE_POS_X, CAPTURE_MODE_POS_Y)
			TEXTURE(TextureCapturePath, capture_path, CAPTURE_PATH_POS_X, CAPTURE_PATH_POS_Y)
		} m_capture_mode_textures;
	}; // class GUI
} // namespace GUI
