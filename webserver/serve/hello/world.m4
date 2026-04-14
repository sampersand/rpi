#!/bin/sh

# This should be configured to use m4 lol

html () { echo "<html>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</html>'; }
head () { echo "<head>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</head>'; }
title () { echo "<title>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</title>'; }
body () { echo "<body>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</body>'; }
h1 () { echo "<h1>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</h1>'; }
p () { echo "<p>"; if [ $# = 0 ]; then cat; else echo $*; fi; echo '</p>'; }

: ${place:=world}

cat <<EOS
<!DOCTYPE html>
`html <<-HTML
	\`head <<-HEAD
		\\\`title Hello $place!\\\`
	HEAD
	\`
	\`body <<-BODY
		\\\`h1 Hello, $place!\\\`
		\\\`p The time on the server is currently \\\\\\\`date\\\\\\\`\\\`
	BODY
	\`
HTML
`
EOS
