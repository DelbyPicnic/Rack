plugin:
	@mkdir -p plugins
	@[ -e plugins/$(DIR) ] && ( echo --- ; read -p "Plugin folder plugins/$(DIR) already exists and will be overwritten. Press Ctrl-C now to cancel or Enter to proceed." ; echo --- ; rm -rf plugins/$(DIR) ) || true
ifeq (,$(findstring http,$(URL)))
	git clone http://github.com/$(URL) plugins/$(DIR) --recursive
else
	git clone $(URL) plugins/$(DIR) --recursive
endif
	make -C plugins/$(DIR)
	@echo ---
	@echo Plugin built.

list-plugins:
	@awk -F ' *\\| *' '{ print "\033[1m"$$1"\033[0m" "\n\t" $$3; if(index($$2,"http")) print "\t"$$2 ; else print "\thttp://github.com/"$$2 ; print ""; }' plugin-list.txt 
	@echo ---
	@echo "Type \"make \033[1mPluginSlug\033[0m\" to install a plugin."
	@echo ---

rebuild-plugins:
	for f in plugins/*; do $(MAKE) -C "$$f"; done

# Special case as it actually installs two plugins
Fundamental:
	URL=VCVRack/Fundamental DIR=Fundamental $(MAKE) plugin
	URL=mi-rack/zFundamental DIR=zFundamental $(MAKE) plugin

PLUGIN = $(shell cat plugin-list.txt | grep -i "^$@ *|")
PLUGIN_DIR = $(strip $(shell echo "$(PLUGIN)" | cut -d "|" -f 1 ))
PLUGIN_URL = $(strip $(shell echo "$(PLUGIN)" | cut -d "|" -f 2 ))

%:
	@if [ ! -n "$(PLUGIN_DIR)" ] ; then echo --- ; echo "No such plugin or target: $@" ; echo "Type \"make list-plugins\" for a list of plugins known to this build script." ; echo --- ; false ; else true ; fi
	URL="$(PLUGIN_URL)" DIR="$(PLUGIN_DIR)" $(MAKE) plugin
