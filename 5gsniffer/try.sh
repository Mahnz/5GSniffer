#!/bin/bash
rm -rf /5gsniffer/logs && mkdir /5gsniffer/logs
SPDLOG_LEVEL=debug /5gsniffer/build/src/5g_sniffer /5gsniffer/configs/Tracker-live.toml > /5gsniffer/logs/debug.out
