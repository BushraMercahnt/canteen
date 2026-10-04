#ifndef AUTH_H
#define AUTH_H
#include "common.h"
/* Staff login. Owner: Partner B */
int auth_check_login(const char *user, const char *pass);   /* 1 = correct */
int auth_is_staff(const Request *req);                      /* 1 = has the staff cookie */
#define AUTH_COOKIE "staff=yes; Path=/; Max-Age=28800"      /* send this with send_redirect() after a good login */
#endif
