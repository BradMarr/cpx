#!/bin/bash
sudo apt install -y clang-19

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y g++-13 libstdc++6

sudo curl -L https://github.com/BradMarr/cpx/releases/latest/download/cpx -o /usr/local/bin/cpx

sudo chmod +x /usr/local/bin/cpx