#!/bin/bash
set -eu
set -o pipefail

target_server="$1"
echo deploying to "$target_server"

revision=$(git describe --match=NeVeRmAtCh --always --abbrev=10 --dirty)
if [[ $revision = *"dirty"* ]]; then
  echo "Git workspace is dirty, commit before deploying"
  exit 1
fi

temp=$(mktemp -d)
pkg_name="pipeline_$revision"
echo "deploying $pkg_name"
echo "working in "$temp""
pipeline_dir="$temp"/"$pkg_name"/
pkg_file="$pkg_name".tar.gz
pkg_path="$temp"/"$pkg_file"
mkdir -p "$pipeline_dir"
echo "creating temp clone"
git clone . "$pipeline_dir"
rm -rf "$pipeline_dir"/.git
echo "creating tarball"
tar -C "$temp" -czf "$pkg_path" "$pkg_name"
echo "copying to server"
scp "$pkg_path" "$target_server:~"
echo "unpacking on server"
ssh "$target_server" "tar -xzf $pkg_file && rm $pkg_file"

#rm -rf "$temp"
