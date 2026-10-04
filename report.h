#ifndef REPORT_H
#define REPORT_H
#include "common.h"
/* Daily report numbers. Owner: Partner B */
void report_totals(int *orders_count, int *sales_total);   /* TODO: total orders and total sales (not cancelled) */
void report_ratings(int *rated_count, double *average);    /* TODO: how many orders were rated and the average stars */
#endif
