This is work-in-progress graphical display server for the Linux kernel.  This
is an attempt to create simpler alternatives to critical system services on
Linux.  At the current state, it's not as much a serious project but more of
an exploration of what could be done different from existing solutions.

The code in this repo is a little rough, much of it is in flux and needs some
cleanup, and there is probably some old code hanging around.

# GOALS #

- Simplified base/low level API
- Optimized for modern/common consumer hardware
- Minimal external dependancies


# BACKGROUND #

I am not comfortable with the amount of complexity that exists in modern
desktop operating systems.  In an effort to simplify my own personal desktop
experience, I wanted to try my hand at making a display server.


# MORE RESOURCES #

- I occasionally work on this and other projects live on Twitch: https://www.twitch.tv/croepha
- I sometimes also announce things on Twitter: https://twitter.com/croepha


# FAQ #

Q: Why not Wayland?

A: I think that Wayland is a significant improvement over Xorg in a number of
ways.  If anything I don't think that Wayland goes far enough.  I think that 
wayland is still significantly more complex than it needs to be.


