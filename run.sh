# run.sh - builds the canteen and starts it. In the w64devkit terminal type:  sh run.sh
taskkill /F /IM shop.exe >/dev/null 2>&1
gcc -Wall -Wextra -o shop.exe main.c server.c router.c page.c menu.c storage.c queue.c auth.c report.c pages_customer.c pages_admin.c scan.c -lws2_32 && ./shop.exe 8090
