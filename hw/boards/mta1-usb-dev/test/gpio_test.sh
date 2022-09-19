#!/bin/bash

pushd app_test
make flash
popd

pushd int_test
make flash
popd
