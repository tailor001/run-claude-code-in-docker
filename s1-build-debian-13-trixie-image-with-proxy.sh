#!/bin/bash
# 构建基于官方镜像的客制化基础镜像
docker build  \
    --network host \
    --build-arg http_proxy="http://127.0.0.1:7890" \
    -f ./dockerfile-debian-13-trixie -t debian-13:tailor .
