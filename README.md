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
install script uses `apt` and `dpkg`, so it only runs on Debian and Ubuntu, but
the packages can be unpacked and run by hand on other distributions.

- The USB service (`tobiiusbservice` 2.1.5) ships udev rules for every product
  ID from `0101` to `05FF`, which includes the Eye Tracker 5's `0313`.
- The service and its three libraries load on a current Arch-based system with
  no missing dependencies.
- The service writes a PID file to `/var/run/tobiiusb/` and needs write access
  to the tracker's USB device. `udev/70-tobii.rules` grants that access to the
  logged-in user.
