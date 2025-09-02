#!/usr/bin/env python3
import sys

sys.path.append('../../..')
from build_all import run
sys.path.pop()


run(['jam', '-sRoot=../../..', '-f', 'jamfile'])

