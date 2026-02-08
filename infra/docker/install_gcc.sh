set -e

add-apt-repository universe
apt update
apt install -y gcc-14 g++-14
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100