FROM gcc:13 AS build
WORKDIR /app
COPY *.c *.h cover.jpg ./
RUN gcc -O2 -o canteen main.c server.c router.c page.c menu.c storage.c queue.c auth.c report.c pages_customer.c pages_admin.c scan.c

FROM debian:stable-slim
WORKDIR /app
COPY --from=build /app/canteen /app/cover.jpg ./
CMD ["sh", "-c", "./canteen ${PORT:-8080}"]
