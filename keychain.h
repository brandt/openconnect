#ifndef OC_KEYCHAIN_H
#define OC_KEYCHAIN_H

char *build_keychain_name(const char *user, const char *domain);
int keychain_add(char **err, const char *service, const char *account, const char *pass);
int keychain_find(char **err, const char *service, const char *account, unsigned int *length, char **password);
int keychain_remove(char **err, const char *service, const char *account);

#endif /* end of include guard: OC_KEYCHAIN_H */
