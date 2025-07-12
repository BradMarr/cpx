wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s -- 19 all
sudo apt install clang-19

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y g++-13 libstdc++6

./cpx build clean