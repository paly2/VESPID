#include <iostream>
#include <exception>
#include <memory>
#include <dirent.h>
#include <cstdlib>
#include <sstream>
#include <cxcore.hpp>

#include "camera.hh"
#include "gpio.hh"
#include "gui.hh"
#include "image.hh"
#include "util.hh"

struct DBException : public std::exception {
	const char* what() const noexcept {
		return "Failed to save file to database.";
	}
};

void captureToDb(GUI::GUI &gui, GUI::captureMode mode, const cv::Mat &image) {
	std::ostringstream path;
	path << "dataset/";
	switch (mode) {
	case GUI::CAPTURE_EMPTY:
		path << "empty/";
		break;
	case GUI::CAPTURE_ASIAN:
		path << "asian/";
		break;
	case GUI::CAPTURE_EUROPEAN:
		path << "european/";
		break;
	default:
		break; // Warning avoidance
	}

	// Compute the index of the picture.
	unsigned int image_n = 0;

	{
		DIR *dpdf;
		struct dirent *epdf;

		dpdf = opendir(path.str().c_str());
		if (dpdf == NULL)
			throw DBException();

		while ((epdf = readdir(dpdf))) {
			unsigned int n = strtol(epdf->d_name, NULL, 10);
			if (n > image_n)
				image_n = n;
		}

		closedir(dpdf);
	}

	++image_n;

	path << image_n << ".ppm";

	cv::Mat image_resized;
	Image::resizeImageForDB(image, image_resized);

	imwrite(path.str(), image_resized);

	gui.updateCapturePath(path.str());
}

int main() {
	try {
		GUI::GUI gui;
		Camera::Camera camera;
		Image::NNManager nn_manager(&camera);
		// By using a unique_ptr, it is easy to delete the GPIO
		// thread in capture mode.
		std::unique_ptr<GPIO::GPIO> gpio(new GPIO::GPIO(&nn_manager));

		cv::Mat image;

		GUI::captureMode mode;

		// Run the loop 30 times a second.
		const int ms_wait = 1000 / 30;

		while (true) {
			unsigned int start_ticks = Time::getTicks();

			GUI::Event event = gui.pollEvent();
			if (event.quit)
				break;

			if (camera.newImage(CAMERA_CLASER_ONSUMER_MAIN_ID)) {
				camera.retrieve(image, CAMERA_CLASER_ONSUMER_MAIN_ID);
				gui.updateImage(image);
			}

			if (nn_manager.newResult(RESULTS_CLASER_ONSUMER_MAIN_ID)) {
				Image::nnResult result;
				result = nn_manager.getResult(RESULTS_CLASER_ONSUMER_MAIN_ID);
				gui.updateNNResult(result);
			}

			if (event.captureMode) {
				mode = static_cast<GUI::captureMode>((mode + 1) % 4);
				gui.setMode(mode);
				if (mode == GUI::NORMAL)
					gpio.reset(new GPIO::GPIO(&nn_manager));
				else
					gpio.reset();
			}

			if (mode == GUI::NORMAL) {
				gui.updateLaser(gpio->getLaserState());
				gui.updateServo(gpio->getServoState());
				if (event.laserOn)
					gpio->simLaserOn();
				if (event.laserOff)
					gpio->simLaserOff();
		 	} else if (event.captureImage) {
				captureToDb(gui, mode, image);
			}

			gui.redraw();

			int diffticks = Time::getTicks() - start_ticks;
			if (ms_wait > diffticks)
				Time::delay(ms_wait - diffticks);
		}
	} catch (std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
