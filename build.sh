# Copyright 2019 David Butler <croepha@gmail.com>

# rm -rvf build

set -eEuo pipefail

bash build_base.cpp


sudo mkdir -p  /tmp/RenderDoc/
sudo chmod a+w /tmp/RenderDoc/

# ./build/test_server.exec

[  ]&& { # To ship...

rm -rf build/ship
mkdir -p build/ship
cp -v \
  HandmadeMath.h \
  test_client.c \
  test_client_multiple_windows.c \
  test_client_opengl.c \
  build/ship/

cp -v build/ds_sdl.exec build/ship/test_server.exec

cp -v ship_readme.text            build/ship/README
cp -v barclay_client_lib.h        build/ship/barclay.h
cp -v build/barclay_client_lib.so build/ship/libbarclay.so


cat << EOF > build/ship/build.sh
#!/bin/sh
cc --std=c89 test_client.c -o test_client.exec -L. -l barclay 
cc --std=c89 test_client_multiple_windows.c -o test_client_multiple_windows.exec -L. -l barclay 
cc --std=c99 test_client_opengl.c -o test_client_opengl.exec -L. -l barclay -lGL -lm
EOF
chmod +x build/ship/build.sh

( cd build/ship/; sh build.sh )

strip build/ship/libbarclay.so
strip build/ship/test_client.exec 
strip build/ship/test_server.exec 
strip build/ship/test_client_multiple_windows.exec 
strip build/ship/test_client_opengl.exec 

mv build/ship build/barclay_alpha_X
tar -cvf build/barclay_alpha_X.tar -C build/ barclay_alpha_X
rm -rvf build/barclay_alpha_X
xz --best --extreme build/barclay_alpha_X.tar


}


