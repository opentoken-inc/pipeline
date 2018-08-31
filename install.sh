. setup.sh
aws --version
if [ "$(uname)" == "Darwin" ]; then
  brew install --upgrade zstd tmux
  brew install getsentry/tools/sentry-cli || brew upgrade getsentry/tools/sentry-cli
else
  sudo yum install -y zstd tmux git python3 python3-devel
  sudo yum groupinstall "Development Tools"
fi
