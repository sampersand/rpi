#!/bin/sh
set -ex

# We should probably figure out hwo to install git and make if not
# already installed

echo == cleaning up old files ==
rm -rf ~/r49
rm -f ~/bin/webserver
sudo rm -f /usr/local/bin/r49 /usr/local/bin/ruby

echo == making ~/bin ==
mkdir -p ~/bin

if false; then
echo == git cloning r49 ==
git clone https://github.com/sampersand/ruby-0.49 ~/r49
(
	echo == building r49 \(may take a bit\) ==
	cd ~/r49/fixed
	./configure >/dev/null
	make -s

	echo == installing r49 ==
	sudo make install
	sudo mv `which ruby | sed 'p;s/ruby/r49/'`
)
fi

echo == making webserver ==
cc webserver.c -o ~/bin/webserver || echo "oops: $?"
