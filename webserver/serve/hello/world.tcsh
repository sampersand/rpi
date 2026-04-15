#!/bin/tcsh

if ( $METHOD != GET ) then
	status 405 true
	exit
endif

if ( ! $?place ) set place = world

alias h  'echo "<\!:*>"'
alias ht 'echo "<\!:1> \!:2* </\!:1>"'

h \!DOCTYPE html
h html lang=en
h head
ht title Hello $place\!
h /head
h body
ht h1 Hello, $place\!
ht p The time on the server is currently `date`.
h /body
h /html
