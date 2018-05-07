#include <string>
#include <vector>
#include <cstdio>
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <cxcore.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "gui.hh"
#include "image.hh"
#include "gpio.hh"
#include "util.hh"

#include "cmake_config.h"

#define DRAW_TEXTURES(textures) drawTextures((Texture*) &(textures), sizeof(textures));
#define DESTROY_TEXTURES(textures) destroyTextures((Texture*) &(textures), sizeof(textures));
#define INIT_TEXTURES(textures) initTextures((Texture*) &(textures), sizeof(textures));

namespace GUI {
	GUI::GUI() {
		/// Initialization
		if (TTF_Init() < 0)
			throw SDLException("Failed to initialize SDL_ttf");
		m_font = TTF_OpenFont(GUI_TTF_FLASER_ONT, GUI_TTF_PTSIZE);
		if (m_font == NULL)
			throw SDLException("Failed to open TTF font");

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			throw SDLException("Failed to initialize SDL");
		// The raspberry pi is not supposed to run anything but this program in
		// the graphical environment.
		m_window = SDL_CreateWindow("Hornet", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
		if (m_window == NULL)
			throw SDLException("Failed to create the window");

		m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (m_renderer == NULL)
			throw SDLException("Failed to create the renderer");

		m_general_textures.init(m_renderer, m_font);
		m_normal_mode_textures.init(m_renderer, m_font);
		m_capture_mode_textures.init(m_renderer, m_font);

		m_general_textures.compile_date.generate();

		/// Draw
		redraw();

		/// Disable cursor
		SDL_ShowCursor(SDL_DISABLE);
	}

	GUI::~GUI() {
		TTF_CloseFont(m_font);
		m_font = NULL;

		TTF_Quit();

		SDL_DestroyRenderer(m_renderer);
		m_renderer = NULL;

		SDL_DestroyWindow(m_window);
		m_window = NULL;

		SDL_Quit();

		// Textures are freed in destructor.
	}

	void GUI::updateImage(const cv::Mat &image) {
		m_general_textures.image.updateFromImage(image);
	}

	void GUI::updateServo(GPIO::servoState servo_state) {
		m_normal_mode_textures.servo.setServoState(servo_state);
	}

	void GUI::updateLaser(GPIO::laserState laser_state) {
		m_normal_mode_textures.laser.setLaserState(laser_state);
	}

	void GUI::updateNNResult(Image::nnResult result) {
		m_normal_mode_textures.result.setNNResult(result);
	}

	void GUI::setMode(captureMode mode) {
		m_mode = mode;
		m_capture_mode_textures.capture_mode.setMode(mode);
	}

	void GUI::updateCapturePath(std::string path) {
		m_capture_mode_textures.capture_path.setCapturePath(path);
	}

	void GUI::redraw() {
		m_general_textures.freq.redrawing();

		SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
		SDL_RenderClear(m_renderer);
		m_general_textures.draw();
		if (m_mode != NORMAL)
			m_capture_mode_textures.draw();
		else
			m_normal_mode_textures.draw();

		SDL_RenderPresent(m_renderer);
	}

	Event GUI::pollEvent() {
		Event event;
		SDL_Event sdl_event;
		while (SDL_PollEvent(&sdl_event)) {
			switch (sdl_event.type) {
			case SDL_KEYDOWN:
				switch (sdl_event.key.keysym.sym) {
				case SDLK_k:
					event.captureMode = true;
					break;
				case SDLK_SPACE:
					event.captureImage = true;
					break;
				case SDLK_l:
					event.laserOn = true;
					break;
				case SDLK_ESCAPE:
					event.quit = true;
					break;
				}
				break;
			case SDL_KEYUP:
				switch (sdl_event.key.keysym.sym) {
				case SDLK_l:
					event.laserOff = true;
					break;
				}
				break;
			case SDL_QUIT:
				event.quit = true;
				break;
			}
		}

		return event;
	}

	SDL_Surface* GUI::matToSurface(const cv::Mat &image) {
		// This function assumes an RGB image with a 24-bits depth.
		SDL_Surface *surface = SDL_CreateRGBSurfaceFrom((void*) image.data,
			image.cols,
			image.rows,
			24,
			image.cols * 3,
			0xff0000, 0x00ff00, 0x0000ff, 0);
		return surface;
	}

	void TextureImage::updateFromImage(const cv::Mat &image) {
		int renderer_width, renderer_height;
		SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);

		cv::Mat image_resized;
		int x_pos = 0, y_pos = 0;

		Image::resizeImageForScreen(image, image_resized, renderer_width, renderer_height, x_pos, y_pos);

		if (!isInitialized()) {
			// TODO: also recreate the texture when the screen size changes
			setXY(x_pos, y_pos);
			recreateEmpty(SDL_PIXELFORMAT_BGR24, image_resized.cols, image_resized.rows);
		}

		updateFromData((void*) image_resized.data, image_resized.cols * 3);
	}

	TextureFreq::TextureFreq(int x, int y, TextureSet *parent_set) : Texture(x, y, parent_set) {
		m_last_freq_ticks = Time::getTicks();
	}

	void TextureFreq::redrawing() {
		unsigned int new_ticks = Time::getTicks();
		m_freq_ms_sum += new_ticks - m_last_freq_ticks;
		m_last_freq_ticks = new_ticks;
		m_freq_ms_number++;

		if (m_freq_ms_sum < 1000)
			return;

		double freq;
		freq = (double) (m_freq_ms_number * 1000) / (double) m_freq_ms_sum;
		m_freq_ms_number = m_freq_ms_sum = 0;

		std::ostringstream freqstr;
		freqstr << freq << " FPS";
		recreateFromText(freqstr.str().c_str(), {255, 255, 0});
	}

	void TextureCompileDate::generate() {
		recreateFromText(PROJECT_NAME " " VERSION_STRING " " __DATE__ " " __TIME__, {255, 255, 0});
	}

	void TextureServo::setServoState(GPIO::servoState servo_state) {
		if (servo_state == m_servo_state)
			return;

		if (servo_state == GPIO::SERVO_DEATH)
			recreateFromText("Servo: Death", {255, 0, 0});
		else
			recreateFromText("Servo: Life", {0, 255, 0});
		m_servo_state = servo_state;
	}

	void TextureLaser::setLaserState(GPIO::laserState laser_state) {
		if (laser_state == m_laser_state)
			return;

		if (laser_state == GPIO::LASER_OFF)
			recreateFromText("Laser: Off", {255, 0, 0});
		else
			recreateFromText("Laser: On", {0, 255, 0});
		m_laser_state = laser_state;
	}

	void TextureNNResult::setNNResult(Image::nnResult result) {
		char txt[100];
		SDL_Color color;

		if (result.empty_prob > result.asian_prob && result.empty_prob > result.european_prob) {
			snprintf(txt, 100, "Empty %d %%", (int) (result.empty_prob * 100));
			color = {255, 255, 255};
		} else if (result.asian_prob > result.european_prob) {
			snprintf(txt, 100, "Asian %d %%", (int) (result.asian_prob * 100));
			color = {255, 0, 0};
		} else {
			snprintf(txt, 100, "European %d %%", (int) (result.european_prob * 100));
			color = {0, 255, 0};
		}

		recreateFromText(txt, color);
	}

	void TextureCaptureMode::setMode(captureMode mode) {
		if (mode != NORMAL) {
			switch (mode) {
			case CAPTURE_EMPTY:
				recreateFromText("CAPTURE MODE ENABLED: CAPTURING EMPTY", {255, 255, 255});
				break;
			case CAPTURE_ASIAN:
				recreateFromText("CAPTURE MODE ENABLED: CAPTURING ASIAN", {255, 255, 255});
				break;
			case CAPTURE_EUROPEAN:
				recreateFromText("CAPTURE MODE ENABLED: CAPTURING EUROPEAN", {255, 255, 255});
				break;
			default:
				break; // Warning avoidance
			}
		}
	}

	void TextureCapturePath::setCapturePath(std::string path) {
		std::ostringstream txt;
		txt << "Saved image to " << path;
		recreateFromText(txt.str().c_str(), {255, 255, 255});
	}

	Texture::Texture(int x, int y, TextureSet *parent_set) : x(x), y(y) {
		parent_set->_addTexture(this);
	}

	Texture::~Texture() {
		destroy();
	}

	bool Texture::isInitialized() {
		return texture != NULL;
	}

	void Texture::setXY(int p_x, int p_y) {
		x = p_x;
		y = p_y;
	}

	void Texture::setRenderer(SDL_Renderer *p_renderer) {
		renderer = p_renderer;
	}

	void Texture::setFont(TTF_Font *p_font) {
		font = p_font;
	}

	void Texture::recreateFromSurface(SDL_Surface* surface) {
		destroy();

		int renderer_width, renderer_height;
		SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);
		rect.w = surface->w;
		rect.h = surface->h;
		rect.x = (x >= 0 ) ? x : renderer_width + x - rect.w;
		rect.y = (y >= 0 ) ? y : renderer_height + y - rect.h;

		texture = SDL_CreateTextureFromSurface(renderer, surface);

		SDL_FreeSurface(surface);

		if (texture == NULL)
			throw SDLException("Failed to create texture");
	}

	void Texture::recreateFromText(const char *str, SDL_Color color) {
		SDL_Surface *txt_surface;
		txt_surface = TTF_RenderText_Solid(font, str, color);
		recreateFromSurface(txt_surface);
	}

	void Texture::recreateEmpty(unsigned int format, int w, int h) {
		destroy();

		texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STATIC, w, h);
		if (texture == NULL)
			throw SDLException("Failed to create empty texture");

		rect.w = w;
		rect.h = h;
		rect.x = x;
		rect.y = y;
	}

	void Texture::updateFromData(void* data, int pitch) {
		if (SDL_UpdateTexture(texture, NULL, data, pitch) < 0)
			throw SDLException("Failed to update texture");
	}

	void Texture::draw() {
		if (texture != NULL)
			SDL_RenderCopy(renderer, texture, NULL, &rect);
	}

	void Texture::destroy() {
		if (texture != NULL) {
			SDL_DestroyTexture(texture);
			texture = NULL;
		}
	}

	void TextureSet::init(SDL_Renderer *renderer, TTF_Font *font) {
		for (auto it = m_textures.begin() ; it != m_textures.end() ; ++it) {
			(*it)->setRenderer(renderer);
			(*it)->setFont(font);
		}
	}

	void TextureSet::draw() {
		for (auto it = m_textures.begin() ; it != m_textures.end() ; ++it)
			(*it)->draw();
	}

	void TextureSet::_addTexture(Texture *texture) {
		m_textures.push_back(texture);
	}
}
