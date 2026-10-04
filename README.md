# Smart Token and Queue Management System for a College Canteen

A website for ordering canteen food with a digital token, written **completely in C** (no other language, library or database).
The C program is its own web server. It prints every page that the browser shows.

**Team:** Partner A (customer side) and Partner B (server and staff side)

## What it does
Customers (students and staff)
- **Cover (welcome) page** with a big Start button
- Home page with big buttons and a **Quiet now / Busy now** badge
- Menu with **veg / non-veg dots**, a **Popular** tag on the best seller, and **Finished** foods faded out
- Place an order and get a large digital token, with orders ahead and an estimated wait
- Live queue page that refreshes by itself
- **Star rating (1 to 5)** once the order is Ready
- Text size buttons (normal / big / huge) for easy reading

Canteen staff
- Login and logout
- Dashboard: Call Next, and one button per order (Start Preparing, Mark Ready, Mark Collected)
- **Stock page:** mark a food Finished or Available
- Daily report: total orders, total sales and the **average star rating**
- **Scan QR page** (button on the dashboard, or /scan): a QR code that opens the canteen on a phone

## How to run
Windows (MinGW gcc):
```
gcc -Wall -Wextra -o canteen.exe main.c server.c router.c page.c menu.c storage.c queue.c auth.c report.c pages_customer.c pages_admin.c scan.c -lws2_32
canteen.exe
```
Linux / Mac:
```
gcc -Wall -Wextra -o canteen main.c server.c router.c page.c menu.c storage.c queue.c auth.c report.c pages_customer.c pages_admin.c scan.c
./canteen
```
Then open http://localhost:8080 . To use another port: `canteen.exe 8090`.
On a phone (same Wi-Fi): http://YOUR-COMPUTER-IP:8080

Staff login: username `staff`, password `canteen123` (page: /admin)

## Files
| File | What it does | Owner |
|---|---|---|
| common.h | Shared structs and constants (the contract) | both |
| page.c / page.h | Shared page look (CSS) and page helpers | both |
| pages_customer.c / .h | Customer pages (including the cover page) | Partner A |
| cover.jpg | Poster picture shown on the cover page (keep it next to the .c files) | Partner A |
| queue.c / .h | Tokens, queue, ratings, counts | Partner A |
| storage.c / .h | Saves orders in orders.dat | Partner A |
| menu.c / .h | Menu from menu.txt, Finished toggle | Partner A |
| server.c / .h | Web server with sockets | Partner B |
| router.c / .h | Connects each URL to its page function | Partner B |
| pages_admin.c / .h | Staff pages | Partner B |
| auth.c / .h | Staff login and cookie check | Partner B |
| report.c / .h | Report numbers | Partner B |
| scan.c / .h | Scan QR page (QR code drawn in plain C) | Partner B |
| main.c | Starts the program | Partner B |

`menu.txt` and `orders.dat` are created when the program runs. They are not stored in the project folder.
Menu file format, one line per food: `name|price|available|veg`  (available: 1 or 0, veg: 1 or 0)

## Documents
The Business Requirements Document (BRD) with the UI/UX design is in the `docs` folder.

## Notes
- It handles one request at a time, which keeps the file writes safe.
- Future scope: online payment, written reviews, Hindi language switch, password hashing.
