# SpaceShooter

To run the game on MacOS or Linux, navigate to the location where you saved the files, then type ./a.out in the terminal.
The Ncurses library is only available for MacOS and Linux, so you can't run the game on Windows.

## Notes

Ncurses is not thread safe, and I didn't know that by the time I was programing the game.
That's why the output looks weird at times!
See: [thread safety ncurses](https://stackoverflow.com/questions/29910562/why-has-no-one-written-a-threadsafe-branch-of-the-ncurses-library)
