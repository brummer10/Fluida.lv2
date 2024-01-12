# Fluida.lv2

Fluidsynth as LV2 plugin 

![Fluida](https://raw.githubusercontent.com/brummer10/Fluida.lv2/master/Fluida.png)

Channel Matrix Editor

![Fluida](https://raw.githubusercontent.com/brummer10/Fluida.lv2/master/Fluida_Channel_matrix.png)

## Dependencies

- libcairo2-dev
- libx11-dev
- lv2-dev
- libfluidsynth-dev


## Build
- git submodule init
- git submodule update
- make
- make install # will install into ~/.lv2 ... AND/OR....
- sudo make install # will install into /usr/lib/lv2

## Binary
Checkout the latest release for binaries compatible with Linux x86_64 or Windows (64bit)

## Modifying your XDG `user-dirs.dirs` file

It is likely that you don't store your soundfonts in the root of your home directory. If you are using Fluida.lv2 frequently then navigating to your soundfont directory every time you create a new instance of Fluida.lv2 can become tedious in which case you should add your soundfonts dir(s) your `~/.config/user-dirs.dirs` file by adding a line such as:

```
XDG_SF2_DIR="/usr/share/sounds/sf2"
```

Replace `SF2` with a label of your choice for paths you add. You will then be able to access the defined path(s) with a single click from the `Places` panel of Fluida's File Selector window.
