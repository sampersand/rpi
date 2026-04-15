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
		h title -T Hello $place! \(tcsh\)
	h /head
	h body
		h h1 -T Hello, $place\! \(tcsh\)
		h p
			echo -n The time on the server is currently
			h span -a 'style="font-style: italic"'
			echo -n `date`.
			h /span
		h /p
		h p -T It\'s a good day today!
	h /body
h /html
