# tobii-linux

Getting a Tobii Eye Tracker 5 working on Linux, and feeding its head and gaze
data into games that run under Wine/Proton (Star Citizen in particular) through
opentrack.

Tobii does not support its consumer trackers on Linux. This repo records what
works, what doesn't, and the code written to fill the gaps. It does not contain
Tobii's binaries, because their licence doesn't allow redistribution.

## The plan

1. See whether Tobii's old Linux software (the USB service and Stream Engine
   3.0.4, written for the 4C) can talk to the Eye Tracker 5.
2. If it can, write a small program that reads head pose and gaze from Stream
   Engine and sends them to opentrack over UDP.
3. Use opentrack's Wine output so the game sees the data as a TrackIR.
4. If step 1 fails, work out the tracker's USB protocol and write an open
   replacement for the USB service.

## Hardware

The Eye Tracker 5 shows up as `2104:0313 Tobii Technology AB EyeChip`.

## Findings so far

The old Linux packages come from a community repackaging at
https://github.com/MoscowskyAnton/tobii_eye_tracker_linux_installer. Its
install script uses `apt` and `dpkg`, so it only runs on Debian and Ubuntu. The
packages can be unpacked into `vendor/` (ignored by git) and run from there on
other distributions.

Tobii's USB service (`tobiiusbservice` 2.1.5) and Stream Engine 3.0.4 work
with the Eye Tracker 5:

- Stream Engine finds the tracker as `tobii-ttp://IS5FF-<serial>` and connects.
  The tracker reports model `IS5_Large_Eyetracker_5`, generation `IS5`.
- The gaze stream runs at about 30 samples a second, but every sample is
  invalid until the screen size is set and the tracker is calibrated.
- The user presence stream works.
- The tracker has no head pose stream. On Windows, head pose for this tracker
  is computed by Tobii's host software, so it has to come from somewhere else
  here.
- Out of the box, the screen size stored on the tracker is a 4 mm by 4 mm
  placeholder. Setting it and calibrating both need a config-level licence,
  which only Tobii's calibration app (Tobii Config, also in the packages)
  carries.
- Once Tobii Config has set the screen size and calibrated, gaze comes through
  as valid screen positions.

The service writes its PID file and sockets under `/var/run`, which needs root.
`scripts/run-sandboxed.sh` avoids that by running the service and a client
program together in a private user and mount namespace with a writable `/run`.
The tracker's USB device must be accessible to your user, which
`udev/70-tobii.rules` arranges.

## Running Tobii Config

Tobii Config 0.1.6 is an Electron app from 2018. Three things stop it running
on a current Arch-based system:

- It needs GTK2 and GConf: `sudo pacman -S gtk2` and `paru -S gconf` (AUR).
- Its copy of Stream Engine asks for an executable stack, and glibc 2.41 and
  later refuse to load such a library after startup. The window stays blank
  and the page logs `cannot enable executable stack as shared object
  requires`. `scripts/clear_execstack.py` clears that flag:
  `scripts/clear_execstack.py vendor/config/opt/tobii_config/resources/resources/libtobii_stream_engine.so`
- It has to run through XWayland, without Chromium's sandbox (inside the
  namespace it appears to run as root) and without the GPU:

```
scripts/run-sandboxed.sh env -u WAYLAND_DISPLAY $PWD/vendor/config/opt/tobii_config/tobii_config --no-sandbox --disable-gpu
```

Adding `--remote-debugging-port=9333` lets you read the page's console errors
over the Chrome debugging protocol, since the app has no developer tools menu.

Set up the screen, then follow the calibration dots. Each dot only bursts once
the tracker sees you looking near it, so the first round can take a while.

## Trying it

1. Install the udev rule:
   `sudo cp udev/70-tobii.rules /etc/udev/rules.d/ && sudo udevadm control --reload && sudo udevadm trigger --attr-match=idVendor=2104`
2. Unpack the packages from the repackaging repo into `vendor/`:
   `vendor/usbservice` (data of the `tobiiusbservice` deb),
   `vendor/stream_engine` (its `lib/include` and `lib/lib`) and
   `vendor/config` (the `tobii_config` deb).
3. Build the test programs in `probe/` against Stream Engine, for example
   `gcc -o build/probe probe/probe.c -Ivendor/stream_engine/include -Lvendor/stream_engine/lib/x64 -ltobii_stream_engine -pthread -Wl,-rpath,$PWD/vendor/stream_engine/lib/x64`.
4. Run one: `scripts/run-sandboxed.sh build/probe 8`.

`probe/probe.c` connects step by step and prints gaze, head pose and presence.
`probe/state.c` prints the screen size, mounting and calibration stored on the
tracker.
