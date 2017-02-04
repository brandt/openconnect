# OpenConnect with Keychain Support

This is a fork of the OpenConnect VPN project that adds Mac OS X Keychain support.


## About

OpenConnect is an SSL VPN client with support for Cisco's AnyConnect SSL VPN and Juniper's Pulse Connect Secure.

This project adds experimental Keychain support for Mac OS X.


## Installation

### Homebrew

To install with Homebrew:

    brew install --HEAD brandt/personal/openconnect-keychain

To avoid collision with the upstream package, the executable is: `openconnect-keychain`

## Manual

The build and install procedures don't differ from the standard OpenConnect package.

When built on Mac OS X, Keychain support is just automatically enabled.

For the sake of completeness, here are the procedures:


### Requirements

Assumes you already have Homebrew installed.

    # Build requirements
    brew install autoconf automake libtool pkg-config
    
    # Runtime Requirements
    brew install gettext gnutls


### Building

First, we have to do some prep work to get the `vpnc-script` setup in advance:

    export INSTALL_DIR="/tmp/openconnect"  # or wherever...
    mkdir -p "$INSTALL_DIR"/{bin,var,etc}
    curl http://git.infradead.org/users/dwmw2/vpnc-scripts.git/blob_plain/6e04e0bbb66c0bf0ae055c0f4e58bea81dbb5c3c:/vpnc-script -o "${INSTALL_DIR}/etc/vpnc-script"
    chmod +rx "${INSTALL_DIR}/etc/vpnc-script"

Then run through the standard build process:

    export LIBTOOLIZE="glibtoolize"
    ./autogen.sh
    ./configure --prefix="$INSTALL_DIR" --localstatedir="${INSTALL_DIR}/var" --with-vpnc-script="${INSTALL_DIR}/etc/vpnc-script" --sbindir="${INSTALL_DIR}/bin" --disable-nls
    make

To install, run:

    make install


## Usage

Usage does not differ from the vanilla OpenConnect except that you will only be prompted for your password once. Subsequent logins will use your keychain password.

Note: Keychain integration is for the primary password only.  Second passphrases (such as those used for two-factor auth) will still need to be entered manually.


## Reference

For a good reference on building OpenConnect on Mac OS X, see the Homebrew formula (`brew cat openconnect`).

The upstream project: http://www.infradead.org/openconnect/

The `openconnect-keychain` Homebrew formula is available here: https://github.com/brandt/homebrew-personal/blob/master/openconnect-keychain.rb


## License

GNU Lesser Public License, version 2.1
