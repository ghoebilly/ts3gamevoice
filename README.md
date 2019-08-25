# TeamSpeak 3 SideWinder Game Voice Plugin

This TeamSpeak 3 plugin provides support for the Microsoft SideWinder Game Voice USB device.

## Getting Started

### Prerequisites

* Microsoft SideWinder Game Voice hockey puck
* Teamspeak 3 client
* Windows 7 or greater

### Installing

1. Download the [release](https://github.com/ghoebilly/ts3gamevoice/releases) corresponding to your Teamspeak client version (see versioning below) and architecture    
	* win32 for 32-bit client
	* win64 for 64-bit client
2. Install the plugin by double-clicking on the **.ts3_plugin** file you just downloaded
    * The ts3_plugin files should have a Teamspeak 3 icon
	* If a message prompts indicating Teamspeak 3 directory cannot be found, launch Teamspeak 3 first
	* If you cannot install the plugin automatically, this is weird but try to extract the .ts3_plugin file (zip) in _TeamSpeak 3 Client\config_

## Building [![Build Status](https://travis-ci.org/ghoebilly/ts3gamevoice.svg?branch=master)](https://travis-ci.org/ghoebilly/ts3gamevoice)

Requires an environment including setupapi and hid libraries (originally from Windows SDK).

#### Recommended development and build environment
* Microsoft Windows 7 or greater (including Windows Server)
* Visual Studio Community
* MSVC C++ x64/x86 (Desktop development with C++)

Note that ts3_plugin packaging is done by a MSBuild task but is just a zip with the dll and the package.ini.

Travis CI validation build uses a Windows CMake + gcc + ninja environment.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/ghoebilly/ts3gamevoice/tags).

The last digit of the version indicate the Teamspeak **API** compatibility : 1.4.23 for API 23.

## Authors

* JoeBilly

See also the list of [contributors](https://github.com/ghoebilly/ts3gamevoice/contributors)  who participated in this project.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* [Simon Inns](https://github.com/simoninns) - For his usbHidCommunication, helped me a lot
* [Jules Blok](https://github.com/Armada651) - For his TeamSpeak 3 helpers functions from [TeamSpeak 3 G-key plugin](https://github.com/Armada651/gkey_plugin)
* [TeamSpeak Systems GmbH](https://github.com/TeamSpeak-Systems) - [ts3client-pluginsdk](https://github.com/TeamSpeak-Systems/ts3client-pluginsdk)