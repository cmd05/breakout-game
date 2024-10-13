# Breakout

Breakout game inspired by [Breakout](https://en.wikipedia.org/wiki/Breakout_(video_game)) on Atari VCS. This game is built with the help of [LearnOpenGL](https://www.learnopengl.com/) with additional features including:
- Powerups to increase / decrease size of the ball
- Powerups to trigger explosions
- Game loss screen
- Fix maximum paddle width on size increase

To try the extra level, replace content of `levels/one.lvl` with `extra.lvl`.

# Building

The game can be built using CMake.
```
cmake -B build
cmake --build build
```

Currently the game uses prebuilt static libraries for the build process.


# Demo
Unmute the video sound for the gameplay music.

https://github.com/user-attachments/assets/7431807b-994c-4718-8363-a427e68e2dfb