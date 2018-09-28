. setup.sh
aws --version
if [ "$(uname)" == "Darwin" ]; then
  brew install --upgrade zstd tmux
  brew install --with-toolchain llvm
  brew install getsentry/tools/sentry-cli || brew upgrade getsentry/tools/sentry-cli
  rustup install stable
  rustup default stable
  cargo install --features=ssl websocat
else
  sudo yum install -y zstd tmux git python3 python3-devel openssl-devel
  sudo yum groupinstall -y "Development Tools"
  sudo amazon-linux-extras install -y rust1
  cargo install --features=ssl websocat
fi
