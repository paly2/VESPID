## *This repository was migrated to [https://git.fwhost.eu](https://git.fwhost.eu/paly2/VESPID) in reponse to acquisition of GitHub by Microsoft.*

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

### Compilation and installation

Run the following commands to install VESPID and the splash screen:
```
git clone https://github.com/paly2/vespid.git
cd vespid
cmake . -DINSTALL_SPLASH=1
make
sudo make install
```

Now, if you want to change the Plymouth splash screen:
```
plymouth-set-default-theme vespid
```

After running `make install`, the `vespid` executable is installed in
`/usr/local/bin` and all other files in `/usr/local/share/vespid`. To start
VESPID, continue to the next section.

### Configuration

VESPID is configured using environment variables. It won't start if at least the
following environment variables exist (all others have default values):

* `LASER_RECEPTOR_PIN`: pin on which the laser receptor is plugged (identified using broadcom number),
* `SERVO_PIN`: pin on which the servo is plugged (identified using broadcom number),
* `SERVO_LIFE`: servo dutycycle for the "life" position (between 1000 and 2000),
* `SERVO_DEATH`: servo dutycycle for the "death" position (between 1000 and 2000).

In capture mode, neither the receptor nor the servo have to be plugged, you must
nonetheless fill them with values.

In systemd, you can write a configuration file and set the environment values using
the `EnvironmentFile` directive.

In normal operation, VESPID should be run in `/usr/local/share/vespid`.

Here is an example of systemd service file for system with Plymouth, a system-wide
Torch installation, a VESPID configuration file at `/etc/vespid.conf` and a
user `vespid` under which the service is run:
```
[Unit]
Description=VESPID service
Requires=pigpiod.service
After=local-fs.target
After=plymouth-quit-wait.service
Before=getty.target

[Service]
Type=simple
User=vespid
WorkingDirectory=/usr/local/share/vespid
ExecStart=/usr/local/bin/vespid
Restart=on-failure
EnvironmentFile=/etc/vespid.conf
Environment="TERM=dumb"

[Install]
WantedBy=basic.target
```

### Neural network training

VESPID cannot work without a neural network. We provide a script for training
a new neural network.

In order to use it, you must first create a large training dataset. Create a
new directory named `dataset` and 3 subdirectories inside it: `asian`, `european`
and `empty`. (`European` can actually be used for any other insect that could
enter the trap but should not be killed).

Export the required environment values and start VESPID in the parent directory
of `dataset`. Press the `K` key to go through modes. Once you are in a capture
mode, press the spacebar to save the image.

Now, you have to split your dataset into the train dataset and the test dataset.
Add a directory level in `dataset` (`train` and `test`) so your dataset
now looks like this (we will try to provide a shell script to automate this operation
soon):

```
.
└── dataset
    ├── test
    │   ├── asian
    │   ├── european
    │   └── empty
    └── train
        ├── asian
        ├── european
        └── empty
```

Each of the `asian`, `european` and `empty` directories being filled with
corresponding pictures of respectively asian hornets, any other insects (and
particularly european hornets), and no insect. Images in `train` are used to
actually train the network, whereas images in `test` are used to test it
afterwards.

Now, run the `torchnn/train.lua` script with LuaJIT (requires Torch). If you
installed VESPID by running `make install`, this means to run:
```
luajit /usr/local/share/vespid/torchnn/train.lua
```

This script will generate a `nnhornet.t7` file that should be placed in the
working directory of VESPID when started in normal mode.

### Licensing

Copyright © 2018, Langrognet Pierre-Adrien <upsilon@langg.net>,
Agnet Florian <florian.agnet@laposte.net>, Gayraud Florian <gayraud.florian2001@gmail.com>,
Massé Nathan <nathan.masse@laposte.net>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
