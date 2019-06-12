#!/bin/bash

set -e

function main()
{
    local dir
    dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

    qemu-arm-static "${dir}/test_aes"
    qemu-arm-static "${dir}/test_ecdh"
    qemu-arm-static "${dir}/test_hmac"
    qemu-arm-static "${dir}/test_ripemd"
    qemu-arm-static "${dir}/test_sha2"
}

main
