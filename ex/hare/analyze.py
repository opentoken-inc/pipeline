import sys
import numpy as np
from collections import defaultdict
from matplotlib import pyplot as plt
from pprint import pprint
from ujson import loads

def process(events):
  wss_events = defaultdict(dict)
  udp_events = defaultdict(dict)
  for evt in events:
    if evt['source'] == 'wss':
      assert evt['t'] not in wss_events[evt['s']]
      wss_events[evt['s']][evt['t']] = evt['epochNanos']
    elif evt['source'] == 'udp':
      assert evt['t'] not in udp_events[evt['s']]
      udp_events[evt['s']][evt['t']] = evt['epochNanos']

  diffs = defaultdict(dict)
  for market, v in wss_events.items():
    for tid, t in v.items():
      if tid in udp_events[market]:
        diffs[market][tid] = (t - udp_events[market][tid]) / 1e9
      else:
        diffs[market][tid] = None

  results = defaultdict(dict)
  for market, v in diffs.items():
    vs = [vv for vv in v.values() if vv]
    plt.figure()
    plt.hist([v for v in vs if abs(v) > 0.01])
    plt.title(market)
    results[market]['mean'] = np.mean(vs)
    results[market]['median'] = np.median(vs)
    results[market]['min'] = np.min(vs)
    results[market]['max'] = np.max(vs)
    results[market]['std'] = np.std(vs)

  print('Positive indicates a time lead/advantage')
  pprint(results)
  plt.show()


def loads_or_skip(l):
  try:
    return loads(l)
  except ValueError:
    return None

def main(argv):
  with open(argv[0]) as f:
    events = [e for e in (loads_or_skip(l) for l in f.readlines()) if e]
  process(events)

if __name__ == '__main__':
  main(sys.argv[1:])
