set -e

wget -qO /usr/local/bin/ninja.gz https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip
gunzip /usr/local/bin/ninja.gz
chmod a+x /usr/local/bin/ninja