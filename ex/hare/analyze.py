import sys
import numpy as np
import seaborn as sns
from collections import defaultdict, OrderedDict
from matplotlib import pyplot as plt
from pprint import pprint
from ujson import loads

from utils import zstd_open

def process(events):
  quantities = defaultdict(OrderedDict)
  wss_events = defaultdict(dict)
  udp_events = defaultdict(dict)
  for evt in events:
    market = evt['s']
    if evt['source'] == 'wss':
      assert evt['t'] not in wss_events[market]
      wss_events[market][evt['t']] = evt['epochNanos']
    elif evt['source'] == 'udp':
      assert evt['t'] not in udp_events[market]
      udp_events[market][evt['t']] = evt['epochNanos']
      quantities[market][evt['t']] = evt['q']

  diffs = defaultdict(OrderedDict)
  for market, v in wss_events.items():
    for tid, t in v.items():
      if tid in udp_events[market]:
        diffs[market][tid] = (t - udp_events[market][tid]) / 1e9
      else:
        diffs[market][tid] = None

  results = defaultdict(dict)
  for market, v in diffs.items():
    v_filtered = [(k, vv) for k, vv in v.items() if vv and abs(vv) > 0.01]
    vs = [vv for _, vv in v_filtered]
    qm = quantities[market]
    qs = [qm[k] for k, vv in v_filtered]
    g = (
        sns.jointplot(np.log(qs), vs).set_axis_labels(
            'log quantity', 'latency (s)'))
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
  events = []
  for path in sorted(argv):
    with zstd_open(path) as f:
      events.extend(e for e in (loads_or_skip(l) for l in f.readlines()) if e)
  process(events)

if __name__ == '__main__':
  main(sys.argv[1:])
