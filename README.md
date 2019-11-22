gnubg_web is a port of [GNU Backgammon](https://www.gnu.org/software/gnubg/) to the web, with nearly all the features of the original.

A Javascript GUI replaces the GTK-based GUI in the original.

Building the source
-------------------

gnubg_web is built using [Emscripten](https://emscripten.org/), which compiles the gnubg C source code and the glib libraries used by it, into Javascript and [Webassembly](https://webassembly.org/).  Webassembly is supported by the latest modern browsers.

Instructions for building and testing:

1. Install [Emscripten](https://emscripten.org/).  Then activate the PATH and other environment variables by running `source /path/to/emscripten/emsdk_env.sh`.  For more information on this step, see the Emscripten documentation. The latest version of Emscripten which has been tested to successfully build `gnubg_web` is 1.39.3.

2. Execute `./build.sh` from within the gnubg_web directory.

3. This will generate several files within a `build` directory.  To test the build locally, start a webserver inside the `build` directory.  If you have Python installed, a simple way is to run `python -m SimpleHTTPServer 8000` from within the `build` directory.  Then go to `http://localhost:8000/gnubg_web.html` from your browser.  Note that opening the `gnubg_web.html` file directly from your browser probably won't work, because of [CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS) restrictions for local files on the latest browsers.  If you really cannot get anything else to work, you can lookup how to disable such restrictions in your browser, but this is not recommended.

What has been modified from the original GNU Backgammon code?
-------------------------------------------------------------

The `gnubg` subdirectory contains the original source code for GNU Backgammon version 1.05.000, with the following changes:

1. Some source files are not needed in the web version, and have been moved to a `removed_files` subdirectory within `gnubg`, which doesn't get compiled when building `gnubg_web`. This includes the GTK-related files (since the GTK GUI has been replaced by a Javascript GUI), `openurl` (for opening files within a browser) and `external` (for connecting to an external player).

2. A few modifications to the original source have been made, primarily to interact with the Javascript GUI properly.  These changes are marked with `#ifdef WEB` or `#ifndef WEB` blocks.

3. The `packaged_files` subdirectory consists of several data files needed.  These are the neural network weights `gnubg.wd`, the one-sided bearoff database `gnubg_os0.bd`, all the match equity tables in the `met` subdirectory, and some startup settings in `.gnubg/gnubgrc` needed by the web version.  Note that due to its size, the two-sided bearoff database `gnubg_ts0.bd` is currently not packaged. However, it is included in the `gnubg` subdirectory. If you wish to package it, you can copy this file to the `packaged_files` subdirectory before building.

4. Sounds have been disabled.  Although it probably wouldn't be difficult to get them working using Javascript, it didn't seem worth the increased binary size (which increases the download time when serving the binary over the web).

The glib source code
--------------------

GNU Backgammon uses the [glib](https://developer.gnome.org/glib/) libraries extensively.  Because Emscripten needs to build everything from scratch, a copy of the glib 2.62.0 source code is included in the subdirectory `glib/glib-2.62.0`, configured to work with Emscripten.  Many of glib's necessary configuration parameters (`#define`s) have been consolidated into `config.h` within `gnubg`.

Not all of `glib` is needed for compiling `gnubg_web`, so some of the `*.c` source files have been renamed as `*.c_do_not_compile`.  If you make some changes that need one of the files, you should rename them back to `*.c`.  However, note that some of the source files -- particularly anything related to Windows, such as `gwin32` --  will not work in the Emscripten environment.
