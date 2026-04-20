SAMPSHELL_REPO         = https://github.com/sampersand/sampshell
R49_REPO               = https://github.com/sampersand/ruby-0.49
LOCAL_INSTALL_LOCATION = /usr/local/bin/
RUBY_049_CODE_DST      = ~/ruby-0.49-src
SAMPSHELL_LOCATION     = ~/sampshell
INSTALL_SYSTEM_DEP     = sudo apt-get install -y --

.PHONY: r49 sampshell p

install: r49 sampshell p

sampshell: $(SAMPSHELL_LOCATION)
$(SAMPSHELL_LOCATION): | git
	git clone $(SAMPSHELL_REPO) $(SAMPSHELL_LOCATION)

p: $(LOCAL_INSTALL_LOCATION)/p
$(LOCAL_INSTALL_LOCATION)/p: | $(SAMPSHELL_LOCATION) ruby
	sudo ln $(SAMPSHELL_LOCATION)/bin/universal/mviz $@

################################################################################
#                                  Ruby 0.49                                   #
################################################################################

r49-clean:
	-rm -rf -- $(RUBY_049_CODE_DST)
	-sudo rm -f $(LOCAL_INSTALL_LOCATION)/r49

# Entirely remove r49's source code before downloading
r49-redownload:
	rm -rf -- $(RUBY_049_CODE_DST)
	$(MAKE) r49-download

# Download the source code; fails if it already exists
r49-download: $(RUBY_049_CODE_DST)
$(RUBY_049_CODE_DST): | git
	git clone $(R49_REPO) $@
	cd $@/fixed && ./configure

r49: $(LOCAL_INSTALL_LOCATION)/r49
$(LOCAL_INSTALL_LOCATION)/r49: $(RUBY_049_CODE_DST) | yacc
	make -C $</fixed ruby
	sudo ln $</fixed/ruby $@


################################################################################
#                             System Dependencies                              #
################################################################################
.PHONY: yacc zsh ruby csh git

yacc zsh ruby csh git:
	@if ! command -v $@ >/dev/null 2>&1; then \
		echo ==[$@ doesnt exist, installing]==; \
		$(INSTALL_SYSTEM_DEP) $@; \
	fi
