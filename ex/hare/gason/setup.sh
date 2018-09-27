set -e
git clone git@github.com:vivkin/gason.git
(cd gason && git checkout b2f9a6c3ed4d05e3af112a007749be43ca1b664a)
temp=$(mktemp -d)
mv gason/src/gason.h $temp/
mv gason/src/gason.cpp $temp/gason.cc
rm -rf ./gason
mv $temp/* .
rmdir $temp
