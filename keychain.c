#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Security/Security.h>
#include <CoreFoundation/CFString.h>
#include "keychain.h"

// See: https://developer.apple.com/library/mac/documentation/Security/Reference/keychainservices/

// Construct keychain item name from the username and domain
char *build_keychain_name(const char *user, const char *domain) {
	char *login_name;
	int size = 0;
	size = asprintf(&login_name, "%s@%s", user, domain);
	if (size == -1) {
		fprintf(stderr, "Error allocating memory for login name\n");
		exit(1);
	}
	return login_name;
}

char *get_keychain_error(OSStatus status) {
	char *buf = malloc(128);
	CFStringRef str = SecCopyErrorMessageString(status, NULL);
	int success = CFStringGetCString(str, buf, 128, kCFStringEncodingUTF8);
	if (success) {
		strncpy(buf, "Unknown error", 128);
	}
	return buf;
}

// Add a new password to the keychain:
int keychain_add(char **err, const char *service, const char *account, const char *pass) {
	OSStatus res = SecKeychainAddGenericPassword(
		NULL,                       // default keychain
		strlen(service), service,   // service name
		strlen(account), account,   // account name
		strlen(pass), pass,         // password
		NULL                        // item reference
	);
	if (res) {
		*err = get_keychain_error(res);
		return -1;
	}
	return 0;
}

// Get a password from the keychain:
int keychain_find(char **err, const char *service, const char *account, unsigned int *length, char **password) {
	// if (length == NULL || password == NULL) {
	// 	return strdup("length == NULL || password == NULL");
	// }
	// SecKeychainItemRef item;
	char *tmp;

	fprintf(stderr, "Looking up %s keychain for: %s\n", service, account);

	OSStatus res = SecKeychainFindGenericPassword(
		NULL,
		strlen(service), service,
		strlen(account), account,
		length, (void **)&tmp,
		NULL
	);

	if (res == errSecSuccess) {
		*password = strndup(tmp, *length);
		SecKeychainItemFreeContent(NULL, tmp);
		return 0;
	} else if (res == errSecItemNotFound) {
		return -1;
	} else {
		*err = get_keychain_error(res);
		return -2;
	}
}

// Remove item from keychain:
int keychain_remove(char **err, const char *service, const char *account) {
	SecKeychainItemRef item;
	OSStatus res = SecKeychainFindGenericPassword(
		NULL,
		strlen(service), service,
		strlen(account), account,
		NULL, NULL,
		&item
	);
	if (res) {
		*err = get_keychain_error(res);
		return -1;
	}

	res = SecKeychainItemDelete(item);
	if (res) {
		*err = get_keychain_error(res);
		return -2;
	}

	return 0;
}
