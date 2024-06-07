#!/bin/bash

bazelisk query --noimplicit_deps --nohost_deps "kind(\"source file\", deps($1))" | grep @ | cut -d/ -f 1 | uniq
