SHELL = /bin/bash

plugin-clone:
	@[ ! -e plugins/$(DIR) ] && true || ( echo -e "---\nPlugin folder plugins/$(DIR) already exists and will be overwritten. Press \033[1my\033[0m to proceed or any other key to skip this plugin.\nOr update the plugin with UPDATE=1 make +$(PLUGIN_DIR)\n---" ; read -n 1 R; [ "$$R" = "y" ] )
	rm -rf plugins/$(DIR)
ifeq (,$(findstring http,$(URL)))
	git clone http://github.com/$(URL) plugins/$(DIR)
else
	git clone $(URL) plugins/$(DIR)
endif
ifneq (,$(TAG))
	cd plugins/$(DIR) && git reset --hard $(TAG)
endif

plugin-pull:
ifneq (,$(TAG))
	cd plugins/$(DIR) && git fetch
	cd plugins/$(DIR) && git reset --hard $(TAG)
else
	cd plugins/$(DIR) && git pull -f
endif

plugin:
	@mkdir -p plugins
	if [ ! "$(UPDATE)" -o ! -e plugins/$(DIR) ] ; then make plugin-clone ; else make plugin-pull ; fi
	cd plugins/$(DIR) && git submodule update --recursive --init
	cd plugins/$(DIR) && $(MAKE) -i -f ../../plugin-list.mk pre-$(DIR)
	$(MAKE) -C plugins/$(DIR)
	cd plugins/$(DIR) && $(MAKE) -i -f ../../plugin-list.mk post-$(DIR)
	@echo ---
	@echo Plugin $(DIR) built.
	@echo ---

pre-Bidoo:
	echo -e "#include <cstddef>\n$$(cat src/dep/gist/src/mfcc/MFCC.h)" > src/dep/gist/src/mfcc/MFCC.h

post-CharredDesert:
	cp -r deps/rack-components/res/* res/

post-DHE-Modules:
	cp -r gui/res .

post-Fundamental:
	cd ../.. && URL=mi-rack/zFundamental DIR=zFundamental $(MAKE) plugin

post-SynthKit:
	cp -r deps/rack-components/res/* res/

pre-trowaSoft:
	perl -pi -e 's/bool\* isDirty/DirtyWrapper* isDirty/' src/TSParamTextField.hpp

ifneq (,$(findstring arm,$(CPU)))
pre-Valley:
	perl -pi -e 's/<pmmintrin.h>/"util\/SSE2NEON.h"/' src/Dexter/QuadOsc.hpp
	perl -pi -e 's/<pmmintrin.h>/"util\/SSE2NEON.h"/' src/Plateau/InterpDelay.hpp
endif


pre-%: ;
post-%: ;

catalog:
	git clone https://github.com/mi-rack/catalog

update-catalog: catalog
	cd catalog && git pull -f

list-plugins: catalog
	for slug in catalog/manifests/*.json ; do echo -e "\033[1m$$(basename $$slug .json)\033[0m" ; jq -r '"\t\(.name)\n\t\(.pluginUrl//.manualUrl//.sourceUrl)\n"' $$slug ; done
	@echo ---
#	@echo -e "[?] - Untested   [-] - Broken   [!] - Has issues   [~] - Looks OK   [+] - Thoroughly tested and optimised (if needed)"
	@echo -e "Type \"make +\033[1mPluginSlug\033[0m +\033[1mPluginSlug\033[0m ...\" to install plugins (note the plus signs)."
	@echo ---

rebuild-plugins:
	for f in plugins/*; do $(MAKE) -C "$$f"; done

deb-plugins:
	for f in plugins/*; do $(MAKE) deb -C "$$f"; done

update-plugins: catalog
	for slug in catalog/manifests/*.json ; do UPDATE=1 EXISTING=1 $(MAKE) +$$(basename $$slug .json) ; done

+all: catalog
	for slug in catalog/manifests/*.json ; do SKIP=1 $(MAKE) +$$(basename $$slug .json) ; done

+upgrade: catalog
	for slug in catalog/manifests/*.json ; do SKIP=1 UPDATE=1 $(MAKE) +$$(basename $$slug .json) ; done

PLUGIN_MF = $(shell find catalog/manifests -iname $*.json)
PLUGIN_DIR = $(shell basename $(PLUGIN_MF) .json)
PLUGIN_URL = $(strip $(shell jq .sourceUrl $(PLUGIN_MF)))
PLUGIN_TAG = $(strip $(shell jq -r '.tag//""' $(PLUGIN_MF)))	
PLUGIN_STATUS = $(strip $(shell jq -r '.status//""' $(PLUGIN_MF)))	

+%: catalog
	@if [ ! -n "$(PLUGIN_DIR)" ] ; then echo --- ; echo "No such plugin: $*" ; echo "Type \"make list-plugins\" for a list of plugins known to this build script." ; echo --- ; false ; else true ; fi
	@if [[ "$(SKIP)" && "$(PLUGIN_STATUS)" =~ skip|broken ]] ; then echo --- ; echo "Skipping $* with status: $(PLUGIN_STATUS)" ; echo "Type make $@ to force installing it." ; echo --- ; false ; else true ; fi
	if [ ! "$(EXISTING)" -o -e plugins/$(PLUGIN_DIR) ] ; then URL=$(PLUGIN_URL) DIR=$(PLUGIN_DIR) TAG=$(PLUGIN_TAG) $(MAKE) plugin ; fi
