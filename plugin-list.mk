SHELL = /bin/bash

plugin:
	@mkdir -p plugins
	@[ ! -e plugins/$(DIR) ] && true || ( echo -e "---\nPlugin folder plugins/$(DIR) already exists and will be overwritten. Press \033[1my\033[0m to proceed or any other key to skip this plugin.\n---" ; read -n 1 R; [ "$$R" = "y" ] )
	rm -rf plugins/$(DIR)
ifeq (,$(findstring http,$(URL)))
	git clone http://github.com/$(URL) plugins/$(DIR) --recursive
else
	git clone $(URL) plugins/$(DIR) --recursive
endif
	$(MAKE) -C plugins/$(DIR)
	@echo ---
	@echo Plugin $(DIR) built.
	@echo ---

list-plugins:
	@awk -F ' *\\| *' '{ if(match($$1,"^#")) next; print "[" $$3 "] \033[1m"$$1"\033[0m" "\n\t" $$4; if(index($$2,"http")) print "\t"$$2 ; else print "\thttp://github.com/"$$2 ; print ""; }' plugin-list.txt 
	@echo ---
	@echo -e "[?] - Untested   [-] - Broken   [!] - Has issues   [~] - Looks OK   [+] - Thoroughly tested and optimised (if needed)"
	@echo -e "Type \"make +\033[1mPluginSlug\033[0m +\033[1mPluginSlug\033[0m ...\" to install plugins (note the plus signs)."
	@echo ---

rebuild-plugins:
	for f in plugins/*; do $(MAKE) -C "$$f"; done

+all:
	for slug in $$(awk -F ' *\\| *' '{ if(match($$1,"^#")) next; print "+" $$1 }' plugin-list.txt) ; do $(MAKE) $$slug ; done

# Special case as it actually builds two plugins
+Fundamental:
	URL=VCVRack/Fundamental DIR=Fundamental $(MAKE) plugin
	URL=mi-rack/zFundamental DIR=zFundamental $(MAKE) plugin

PLUGIN = $(shell cat plugin-list.txt | grep -i "^$* *|")
PLUGIN_DIR = $(strip $(shell echo "$(PLUGIN)" | cut -d "|" -f 1 ))
PLUGIN_URL = $(strip $(shell echo "$(PLUGIN)" | cut -d "|" -f 2 ))

+%:
	@if [ ! -n "$(PLUGIN_DIR)" ] ; then echo --- ; echo "No such plugin: $*" ; echo "Type \"make list-plugins\" for a list of plugins known to this build script." ; echo --- ; false ; else true ; fi
	URL="$(PLUGIN_URL)" DIR="$(PLUGIN_DIR)" $(MAKE) plugin
	