#!/bin/bash
sudo apt install -y clang-19

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y g++-13 libstdc++6

sudo curl -L $(curl -s https://api.github.com/repos/BradMarr/cpx/releases/latest \
    | grep "browser_download_url" \
    | grep -E '/cpx"' \
    | cut -d '"' -f 4) -o /usr/local/bin/cpx

sudo chmod +x /usr/local/bin/cpx