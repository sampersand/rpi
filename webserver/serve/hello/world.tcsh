#!/bin/tcsh

if ( $METHOD != GET ) then
	status 405 true
	exit
endif

if ( ! $?place ) set place = world

# Also showcases the `h` command without using `[`s
h DOCTYPE
h html -a leng=en
	h head
		h title -T Hello $place!
	h /head
	h body
		h h1 -T Hello, $place\!
		h p -T The time on the server is currently `date`.
	h /body
h /html
