#include "asset.hpp"
#include "util/common.hpp"

namespace mirack {


/** Returns the path of a global resource. Should only read files from this location. */
std::string assetGlobal(std::string filename);
/** Returns the path of a local resource. Can read and write files to this location. */
std::string assetLocal(std::string filename);
/** Returns the path of a hidden resource. This location is used to store settings and stuff thart shouldn't clutter documents folder. */
std::string assetHidden(std::string filename);


} // namespace mirack

namespace rack {


std::string assetGlobal(std::string filename) {
	return mirack::assetGlobal(filename);
}


std::string assetLocal(std::string filename) {
	return mirack::assetLocal(filename);
}


std::string assetPlugin(Plugin *plugin, std::string filename) {
	assert(plugin);
	return plugin->path + "/" + filename;
}


} // namespace rack
