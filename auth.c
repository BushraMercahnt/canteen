#include "auth.h"
#include "server.h"

/* One fixed account is enough for this project. */
int auth_check_login(const char *user, const char *pass)
{
    return strcmp(user, "staff") == 0 && strcmp(pass, "canteen123") == 0;
}

int auth_is_staff(const Request *req)
{
    char v[16];
    return get_cookie(req, "staff", v, sizeof v) && strcmp(v, "yes") == 0;
}
