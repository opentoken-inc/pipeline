. setup.sh
aws --version
if [ "$(uname)" == "Darwin" ]; then
  brew install zstd getsentry/tools/sentry-cli tmux
else
  sudo yum install -y zstd tmux git python3 python3-devel
  sudo yum groupinstall "Development Tools"
fi
