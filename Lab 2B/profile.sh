#!/bin/bash

rm -f profile.gperf
LD_PRELOAD=/u/ee/ugrad/gurbir/lib/libprofiler.so.0 CPUPROFILE=profile.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
pprof --text ./lab2_list ./profile.gperf > profile.out
pprof --list=thread_needle ./lab2_list ./profile.gperf >> profile.out
rm -f ./profile.gperf lab2_list


