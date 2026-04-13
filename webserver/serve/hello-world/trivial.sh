#!/bin/sh

# This is a hello world with a simple `cat` program, and only minimal
# outside batteries

# Make sure it's a GET request; `$METHOD` is provided by the harness
if [ $METHOD != GET ]; then
	status 405 true
	exit 1
fi

# post request params are set as env vars; if `$place` wasn't provided, default
# to `world`.
: ${place:=world}

# Set the content type
header Content-Type text/html

cat <<EOS
<!DOCTYPE html>
<html lang=en>
<head>
	<title>Hello $place!</title>
</head>
<body>
	<h1>Hello, $place!</h1>
	<p>The time on the server is currently `date`.</p>
</body>
</html>
EOS
