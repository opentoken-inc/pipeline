import sys
import numpy as np
from collections import defaultdict
from pprint import pprint
from ujson import loads

def process(events):
  wss_events = defaultdict(dict)
  udp_events = defaultdict(dict)
  for evt in events:
    if evt['source'] == 'wss':
      wss_events[evt['s']][evt['t']] = evt['epochNanos']
    elif evt['source'] == 'udp':
      udp_events[evt['s']][evt['t']] = evt['epochNanos']

  diffs = defaultdict(dict)
  for source, v in wss_events.items():
    for tid, t in v.items():
      if tid in udp_events[source]:
        diffs[source][tid] = (t - udp_events[source][tid]) / 1e6
      else:
        diffs[source][tid] = None

  pprint(diffs)

def main(argv):
  with open(argv[0]) as f:
    events = [loads(l) for l in f.readlines()]
  process(events)

if __name__ == '__main__':
  main(sys.argv[1:])
