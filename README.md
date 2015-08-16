# OpenConnect with Keychain Support

This is a fork of the OpenConnect VPN project that adds Mac OS X Keychain support.

It's still very hackish.  Use with caution.


## About

OpenConnect is an SSL VPN client with support for Cisco's AnyConnect SSL VPN and Juniper's Pulse Connect Secure.

This project adds experimental Keychain support for Mac OS X.


## Installation

The build and install procedures don't differ from the standard OpenConnect package.

When built on Mac OS X, Keychain support is just automatically enabled.

For the sake of completeness, here are the procedures:


### Requirements

Assumes you already have Homebrew installed.


#### Build Requirements

    brew install autoconf automake libtool pkg-config


#### Runtime Requirements

    brew install gettext gnutls


### Building

First we have to do some prep work to get the `vpnc-script` setup in advance:

    export INSTALL_DIR="/tmp/openconnect"  # or wherever...
    mkdir -p "${INSTALL_DIR}/etc" "${INSTALL_DIR}/bin" "${INSTALL_DIR}/var"
    curl http://git.infradead.org/users/dwmw2/vpnc-scripts.git/blob_plain/a64e23b1b6602095f73c4ff7fdb34cccf7149fd5:/vpnc-script -o "${INSTALL_DIR}/etc/vpnc-script"
    chmod +rx "${INSTALL_DIR}/etc/vpnc-script"

Now we can run through the (mostly) standard build process:

    export LIBTOOLIZE="glibtoolize"
    ./autogen.sh
    ./configure --prefix="$INSTALL_DIR" --localstatedir="${INSTALL_DIR}/var" --with-vpnc-script="${INSTALL_DIR}/etc/vpnc-script" --sbindir="${INSTALL_DIR}/bin" --disable-nls
    make

Then to install, just run:

    make install


## Usage

Usage does not differ from the vanilla OpenConnect except that you will only be prompted for your password once. Subsequent logins will use your keychain password.

Note: Keychain integration is for the primary password only.  Second passphrases (such as those used for two-factor auth) will still need to be entered manually.


## Reference

For a good reference on building OpenConnect on Mac OS X, see the Homebrew formula (`brew cat openconnect`).

The upstream project: http://www.infradead.org/openconnect/


## License

GNU Lesser Public License, version 2.1
