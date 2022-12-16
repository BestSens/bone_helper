#!/bin/sh
mkdir -p /tmp/boost_1_81_0
cd /tmp/boost_1_81_0
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar xzf boost_1_81_0.tar.gz
cd boost_1_81_0
./bootstrap.sh || exit 1
sudo ./b2 install || exit 1
cd /tmp
sudo rm -Rf boost_1_81_0