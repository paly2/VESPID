# VESPID
#### Very Effective System for Pest Insects to Die

VESPID is a smart hornet trap that only kills asian hornets (*Vespidae* is the
family of hornets and wasps) and lets european hornets live. This repository
contains the VESPID software source code.

Requires:
* [SDL 2.0](http://libsdl.org/)
* [SDL_tff 2.0](https://www.libsdl.org/projects/SDL_ttf/)
* [pigpio](http://abyz.me.uk/rpi/pigpio/index.html)
* [RaspiCam](https://sourceforge.net/projects/raspicam/)
* [LuaJIT](http://luajit.org/)
* [Torch 7](http://torch.ch)
* [OpenCV](https://opencv.org)

More documentation about making the trap to come.

### Controls

* `Echap`: exit
* `K`: switch capture mode (press again to change captured type), for database
  creation
* `L`: simulate light sensor triggering
* `Space`: take a picture into the database (in capture mode, no effect in normal mode)

### Remarks

The code is optimized to run on a Raspberry Pi 2 model B or better. Video streaming
is extremely slow (and the larger the screen, the slower the rendering) -- probably
about 5 frames per second on a RPi 2. On a RPi 3, the rendering will be a lot
faster -- around 15 frames per second. (However, it seems that the camera can't
grab more than 10 frames per second).

The process runs four threads (in addition to threads automatically created
by SDL):

* The main thread renders the GUI and handles keyboard events,
* The camera thread continously grabs new images from the camera,
* The GPIO thread manages the GPIO (it also directly communicates with the
  image recognition thread because the main thread slowness would consume too
  much time if it had to make the bridge -- perharps 200 milliseconds),
* The image processing thread proceeds image and continously informs the GPIO
  thread of the current hornet category (european, asian, or empty).

The trap operation is the following: as soon as the light sensor is triggered
(by a hornet interposing between the laser and the light sensor), the image
processing threads starts processing incoming images. Once 4 images (configurable)
have been analysed, if the hornet is recognized as asian with an average probability
of at least 70% (configurable), the trap exit door is set to "death" position.
It is placed back in "life" position once all images analysed during 5 seconds
(configurable) have been recognized as "empty".

The process draws directly on the framebuffer (via SDL) and does not allow access
to virtual console unless stopped. It is recommended to run it as the last Systemd
service to be started.

### Licensing

Copyright © 2018, Langrognet Pierre-Adrien <upsilon@langg.net>,
Agnet Florian <florian.agnet@laposte.net>, Gayraud Florian <gayraud.florian2001@gmail.com>,
Massé Nathan <nathan.masse@laposte.net>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
