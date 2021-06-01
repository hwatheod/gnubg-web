FILELIST="gnubg_web.html help.html graphics.js"
mkdir -p build
emcc gnubg/*.c gnubg/lib/*.c glib/glib-2.62.0/glib/*.c glib/glib-2.62.0/glib/libcharset/*.c -O2 -o build/gnubg.js --preload-file packaged_files@/ -s 'EXPORTED_RUNTIME_METHODS=["getValue", "setValue"]' -s ALLOW_MEMORY_GROWTH=1 -DGLIB_COMPILATION=1 -DWEB=1 -I glib/glib-2.62.0/glib/ -I glib/glib-2.62.0/ -I glib/glib-2.62.0/_build/glib -I gnubg/lib/ -I gnubg/ -I glib/glib-2.62.0/_build/ -I glib/glib-2.62.0/glib/libcharset/

# Hack the getpwuid function since it's currently stubbed out and throws an exception
# https://github.com/emscripten-core/emscripten/issues/13219
sed -e 's/throw"getpwuid: TODO"/return 0/g' < build/gnubg.js > build/gnubg_fixed.js
rm build/gnubg.js
mv build/gnubg_fixed.js build/gnubg.js

for file in $FILELIST; do rm -f build/$file; ln -s ../$file build/$file; done;

