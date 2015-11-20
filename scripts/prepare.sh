#!/bin/bash

# S3Common.h S3Downloader.h utils.h utils.cpp S3Downloader.cpp S3Common.cpp extlib/http_parser.cpp include/http_parser.h .
ln -sf sfetch/S3Common.h S3Common.h
ln -sf sfetch/S3Downloader.h S3Downloader.h
ln -sf sfetch/utils.h utils.h
ln -sf sfetch/utils.cpp utils.cpp
ln -sf sfetch/S3Downloader.cpp S3Downloader.cpp
ln -sf sfetch/S3Common.cpp S3Common.cpp
ln -sf sfetch/extlib/http_parser.cpp http_parser.cpp
ln -sf sfetch/include/http_parser.h http_parser.h


