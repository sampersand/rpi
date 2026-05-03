SAMPSHELL_REPO         = https://github.com/sampersand/sampshell
R49_REPO               = https://github.com/sampersand/ruby-0.49
KNIGHT_C_REPO          = https://github.com/knight-lang/c
LOCAL_INSTALL_LOCATION = /usr/local/bin
CODE_SRC_DST           = ~/code-src
SAMPSHELL_LOCATION     = ~/sampshell
INSTALL_SYSTEM_DEP     = sudo apt-get install -y --

R49_RUBYLIB_DST_LOCATION = /usr/local/lib/ruby
R49_RUBYLIB_SRC_LOCATION = ~/shared-files/r49-include

.PHONY: r49 sampshell p h

install: r49 sampshell p h knight

sampshell: $(SAMPSHELL_LOCATION)
$(SAMPSHELL_LOCATION): | git
	git clone $(SAMPSHELL_REPO) $(SAMPSHELL_LOCATION)

p: $(LOCAL_INSTALL_LOCATION)/p
$(LOCAL_INSTALL_LOCATION)/p: | $(SAMPSHELL_LOCATION) ruby
	sudo ln $(SAMPSHELL_LOCATION)/bin/universal/mviz $@

$(CODE_SRC_DST):
	mkdir -p $(CODE_SRC_DST)

################################################################################
#                                  Ruby 0.49                                   #
################################################################################
.PHONY: r49 r49-clean r49-redownload r49-download r49-utils

r49-clean:
	-rm -rf -- $(CODE_SRC_DST)/r49
	-sudo rm -f $(LOCAL_INSTALL_LOCATION)/r49 $(R49_RUBYLIB_DST_LOCATION)

# Entirely remove r49's source code before downloading
r49-redownload:
	rm -rf -- $(CODE_SRC_DST)/r49
	$(MAKE) r49-download

# Download the source code; fails if it already exists
r49-download: $(CODE_SRC_DST)/r49 | $(CODE_SRC_DST)
$(CODE_SRC_DST)/r49: | git
	git clone $(R49_REPO) $@
	cd $@/fixed && ./configure

r49: $(LOCAL_INSTALL_LOCATION)/r49 r49-utils
$(LOCAL_INSTALL_LOCATION)/r49: $(CODE_SRC_DST)/r49 | yacc
	make -C $</fixed ruby
	sudo ln $</fixed/ruby $@

r49-utils: $(R49_RUBYLIB_DST_LOCATION)
$(R49_RUBYLIB_DST_LOCATION): $(R49_RUBYLIB_SRC_LOCATION)
	sudo ln -s $(R49_RUBYLIB_SRC_LOCATION) $(R49_RUBYLIB_DST_LOCATION)

################################################################################
#                             System Dependencies                              #
################################################################################
.PHONY: yacc zsh ruby tcsh git

yacc zsh ruby tcsh git:
	@if ! command -v $@ >/dev/null 2>&1; then \
		echo ==[$@ doesnt exist, installing]==; \
		$(INSTALL_SYSTEM_DEP) $@; \
	fi

################################################################################
#                                    Knight                                    #
################################################################################
.PHONY: knight knight-clean knight-download

knight-clean:
	-rm -rf -- $(CODE_SRC_DST)/knight
	-sudo rm -f $(LOCAL_INSTALL_LOCATION)/knight

knight-redownload:
	rm -rf -- $(CODE_SRC_DST)/knight
	$(MAKE) knight-download

# Download the source code; fails if it already exists
knight-download: $(CODE_SRC_DST)/knight | $(CODE_SRC_DST)
$(CODE_SRC_DST)/knight: | git
	git clone $(KNIGHT_C_REPO) $@

knight: $(LOCAL_INSTALL_LOCATION)/knight
$(LOCAL_INSTALL_LOCATION)/knight: $(CODE_SRC_DST)/knight
	KN_DEFINES='-DKN_EXT_SYSTEM' make -C $</ast
	sudo ln -s $</ast/bin/knight $@

################################################################################
#                                  Webserver                                   #
################################################################################
ARGS ?=

.PHONY: run-webserver
run-webserver: | r49 tcsh knight
	# TODO: pkill
	cd ~/webserver && ./webserver.r49 $(ARGS)
