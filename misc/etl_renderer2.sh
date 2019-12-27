#!/bin/sh
# Simple script to start ET Legacy client with experimental new renderer
#

ETL_BIN=etl

if [ ! -f etl.app ]; then
    ETL_BIN=etl.app/Contents/MacOS/etl
fi

./$ETL_BIN +set cl_renderer opengl2 +set com_hunkmegs 512  
