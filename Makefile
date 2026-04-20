PI_USERNAME := sampersand
PI_HOST     := samperpi.local
PI_PASSWORD := $(shell security find-generic-password -a $(PI_USERNAME) -s $(PI_HOST) -w)
SP          := sshpass -p "$(PI_PASSWORD)"

.PHONY: deploy connect

# rclean:
# 	@$(SP) ssh "$(PI_HOST)" 'bash -s' <<-'EOF'
# 		echo == cleaning up old files ==
# 		p $PW
# 		rm -rf ~/r49
# 		rm -f ~/bin/webserver
# 		sudo rm -f /usr/local/bin/r49 /usr/local/bin/ruby
# 	EOF

connect:
	@$(SP) ssh "$(PI_USERNAME)@$(PI_HOST)"

deploy: copy-files initial-startup
copy-files:
	@echo "== copying files over =="
	@$(SP) scp -r ./webserver $(PI_HOST):~/
	@$(SP) scp -r ./deploy/* $(PI_HOST):~/
	@#$(SP) scp -r ./pi-initial-setup.sh $(PI_HOST):~/
	@# $(SP) scp -r ./copy/usr-bin/. $(PI_HOST):/tmp/bin
	@# $(SP) ssh "$(PI_HOST)" "sudo mv /tmp/bin/* /usr/bin"

initial-startup:
	@echo "== running initial startup script =="
	@#$(SP) ssh "$(PI_HOST)" 'sh ~/pi-initial-setup.sh'

webserver:
	@$(SP) ssh "$(PI_HOST)" 'pkill webserver; ~/bin/webserver 2>&1'
